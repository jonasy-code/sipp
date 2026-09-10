/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright Equivalent Software HB  1992
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 1, or any later version.
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** You can receive a copy of the GNU General Public License from the
 ** Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

/**
 ** sipp_png.c - Writing images as PNG files.
 **
 ** This is a complete, self-contained PNG encoder, so that SIPP does not
 ** depend on libpng or zlib.  It consists of:
 **
 **   - adaptive scanline filtering (the filter of each row is chosen
 **     with the heuristic recommended by the PNG specification),
 **   - a deflate compressor: LZ77 matching over a 32K window with hash
 **     chains and lazy evaluation, emitted with the fixed Huffman codes
 **     of RFC 1951 (block type 1),
 **   - the zlib framing (RFC 1950) and PNG chunk structure around it.
 **
 ** Fixed Huffman codes compress somewhat worse than the dynamic codes a
 ** full zlib produces (typically 10-25% larger files for rendered
 ** images), but the encoder is a fraction of the size and needs no
 ** tables to be built and transmitted.
 **/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sipp.h>

#include <sipp_png.h>
#include <smalloc.h>

/*======================== Checksums =========================*/

static unsigned long crc_table[256];

/*
 * Fixed Huffman codes for the literal/length alphabet, stored with the
 * bits already reversed so they can be written LSB first like all
 * other deflate fields.
 */
static unsigned short lit_code[288];
static unsigned char lit_len[288];
static unsigned char dist_code[30];

static unsigned reverse_bits(unsigned code, int len) {
  unsigned r = 0;

  while (len-- > 0) {
    r = (r << 1) | (code & 1);
    code >>= 1;
  }
  return r;
}

static void tables_fill(void) {
  unsigned long c;
  int n, k;

  for (n = 0; n < 256; n++) {
    c = (unsigned long)n;
    for (k = 0; k < 8; k++) {
      c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
    }
    crc_table[n] = c;
  }

  /* RFC 1951, 3.2.6 */
  for (n = 0; n < 288; n++) {
    if (n < 144) {
      lit_len[n] = 8;
      lit_code[n] = reverse_bits(0x30 + n, 8);
    } else if (n < 256) {
      lit_len[n] = 9;
      lit_code[n] = reverse_bits(0x190 + n - 144, 9);
    } else if (n < 280) {
      lit_len[n] = 7;
      lit_code[n] = reverse_bits(n - 256, 7);
    } else {
      lit_len[n] = 8;
      lit_code[n] = reverse_bits(0xC0 + n - 280, 8);
    }
  }
  for (n = 0; n < 30; n++) {
    dist_code[n] = reverse_bits(n, 5);
  }
}

static void tables_init(void) {
  static pthread_once_t once = PTHREAD_ONCE_INIT;

  pthread_once(&once, tables_fill);
}

static unsigned long crc_update(unsigned long crc, const unsigned char *buf,
                                size_t len) {
  size_t i;

  for (i = 0; i < len; i++) {
    crc = crc_table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
  }
  return crc;
}

enum {
  ADLER_BASE = 65521,
  ADLER_NMAX = 5552 /* Largest n with 255n(n+1)/2 + (n+1)(BASE-1) < 2^32 */
};

static unsigned long adler32(const unsigned char *buf, size_t len) {
  unsigned long a = 1, b = 0;
  size_t n;

  while (len > 0) {
    n = (len < ADLER_NMAX) ? len : ADLER_NMAX;
    len -= n;
    while (n-- > 0) {
      a += *buf++;
      b += a;
    }
    a %= ADLER_BASE;
    b %= ADLER_BASE;
  }
  return (b << 16) | a;
}

/*==================== Growable output buffer ======================*/

typedef struct {
  unsigned char *data;
  size_t len;
  size_t cap;
  unsigned bitbuf; /* Bits not yet written, LSB first */
  int nbits;
} Out_buf;

static void out_init(Out_buf *o, size_t cap) {
  o->data = (unsigned char *)smalloc(cap);
  o->len = 0;
  o->cap = cap;
  o->bitbuf = 0;
  o->nbits = 0;
}

static void out_byte(Out_buf *o, unsigned char c) {
  if (o->len == o->cap) {
    o->cap *= 2;
    o->data = (unsigned char *)srealloc(o->data, o->cap);
  }
  o->data[o->len++] = c;
}

/*
 * Append the N low bits of VALUE, least significant bit first.
 * N is at most 16.
 */
static void put_bits(Out_buf *o, unsigned value, int n) {
  o->bitbuf |= value << o->nbits;
  o->nbits += n;
  while (o->nbits >= 8) {
    out_byte(o, (unsigned char)(o->bitbuf & 0xff));
    o->bitbuf >>= 8;
    o->nbits -= 8;
  }
}

static void flush_bits(Out_buf *o) {
  if (o->nbits > 0) {
    out_byte(o, (unsigned char)(o->bitbuf & 0xff));
    o->bitbuf = 0;
    o->nbits = 0;
  }
}

/*========================= Deflate ===========================*/

enum {
  WINDOW_SIZE = 32768,
  WINDOW_MASK = WINDOW_SIZE - 1,
  HASH_BITS = 15,
  HASH_SIZE = 1 << HASH_BITS,
  MIN_MATCH = 3,
  MAX_MATCH = 258,
  MAX_CHAIN = 32,    /* Longest hash chain to follow */
  NICE_LENGTH = 128, /* A match this long is good enough */
  MAX_LAZY = 32      /* Only look for a better match at the next
                        position if the current one is shorter */
};

/* RFC 1951, 3.2.5 */
static const unsigned short len_base[29] = {
    3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
    31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const unsigned char len_extra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                            1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                            4, 4, 4, 4, 5, 5, 5, 5, 0};
static const unsigned short dist_base[30] = {
    1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,
    97,  129, 193, 257, 385, 513,  769,  1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577};
static const unsigned char dist_extra[30] = {0, 0, 0, 0, 1, 1, 2,  2,  3,  3,
                                             4, 4, 5, 5, 6, 6, 7,  7,  8,  8,
                                             9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static void put_literal(Out_buf *o, int lit) {
  put_bits(o, lit_code[lit], lit_len[lit]);
}

static void put_match(Out_buf *o, int len, int dist) {
  int lc, dc;

  for (lc = 28; len < len_base[lc]; lc--)
    ;
  put_bits(o, lit_code[257 + lc], lit_len[257 + lc]);
  if (len_extra[lc] > 0) {
    put_bits(o, (unsigned)(len - len_base[lc]), len_extra[lc]);
  }

  for (dc = 29; dist < dist_base[dc]; dc--)
    ;
  put_bits(o, dist_code[dc], 5);
  if (dist_extra[dc] > 0) {
    put_bits(o, (unsigned)(dist - dist_base[dc]), dist_extra[dc]);
  }
}

typedef struct {
  const unsigned char *in;
  long n;
  long *head; /* Most recent position with each hash, or -1 */
  long *prev; /* Previous position with the same hash, by
                 position modulo the window size */
} Lz_state;

static unsigned hash3(const unsigned char *p) {
  unsigned v = ((unsigned)p[0] << 16) | ((unsigned)p[1] << 8) | p[2];

  return (v * 2654435761u) >> (32 - HASH_BITS);
}

static void lz_insert(Lz_state *lz, long pos) {
  unsigned h;

  if (pos + MIN_MATCH > lz->n) {
    return;
  }
  h = hash3(lz->in + pos);
  lz->prev[pos & WINDOW_MASK] = lz->head[h];
  lz->head[h] = pos;
}

/*
 * Find the longest match for the data at POS among the earlier
 * positions in the window.  Returns its length (0 if shorter than
 * MIN_MATCH) and its distance in *DIST.  POS itself must not have been
 * inserted yet.
 */
static int longest_match(Lz_state *lz, long pos, int *dist) {
  const unsigned char *in = lz->in;
  long cand, limit;
  int chain, best, max_len, l;

  if (pos + MIN_MATCH > lz->n) {
    return 0;
  }
  max_len = (int)((lz->n - pos < MAX_MATCH) ? lz->n - pos : MAX_MATCH);
  limit = pos - WINDOW_SIZE;
  if (limit < 0) {
    limit = 0;
  }
  best = 0;
  cand = lz->head[hash3(in + pos)];
  for (chain = MAX_CHAIN; chain > 0 && cand >= limit; chain--) {
    if (in[cand + best] == in[pos + best] && in[cand] == in[pos]) {
      for (l = 0; l < max_len && in[cand + l] == in[pos + l]; l++)
        ;
      if (l > best) {
        best = l;
        *dist = (int)(pos - cand);
        if (best >= NICE_LENGTH || best >= max_len) {
          break;
        }
      }
    }
    cand = lz->prev[cand & WINDOW_MASK];
  }
  return (best >= MIN_MATCH) ? best : 0;
}

/*
 * Compress N bytes at IN into O as one deflate block with fixed
 * Huffman codes.
 */
static void deflate_fixed(Out_buf *o, const unsigned char *in, long n) {
  Lz_state lz;
  long pos, i;
  int len, dist, len2, dist2;

  lz.in = in;
  lz.n = n;
  lz.head = (long *)smalloc(HASH_SIZE * sizeof(long));
  lz.prev = (long *)smalloc(WINDOW_SIZE * sizeof(long));
  for (i = 0; i < HASH_SIZE; i++) {
    lz.head[i] = -1;
  }
  for (i = 0; i < WINDOW_SIZE; i++) {
    lz.prev[i] = -1;
  }

  put_bits(o, 1, 1); /* BFINAL */
  put_bits(o, 1, 2); /* BTYPE = 01, fixed Huffman codes */

  dist = 0;
  pos = 0;
  len = longest_match(&lz, pos, &dist);
  while (pos < n) {
    lz_insert(&lz, pos);

    if (len == 0) {
      put_literal(o, in[pos]);
      pos++;
      len = longest_match(&lz, pos, &dist);
      continue;
    }

    /*
     * Lazy evaluation: if the next position starts a longer match,
     * emit this byte as a literal and take that one instead.
     */
    if (len < MAX_LAZY && pos + 1 < n) {
      len2 = longest_match(&lz, pos + 1, &dist2);
      if (len2 > len) {
        put_literal(o, in[pos]);
        pos++;
        len = len2;
        dist = dist2;
        continue;
      }
    }

    put_match(o, len, dist);
    for (i = 1; i < len; i++) {
      lz_insert(&lz, pos + i);
    }
    pos += len;
    len = longest_match(&lz, pos, &dist);
  }

  put_bits(o, lit_code[256], lit_len[256]); /* End of block */
  flush_bits(o);

  sfree(lz.head);
  sfree(lz.prev);
}

/*
 * Wrap the deflate stream of IN in the zlib format (RFC 1950).
 */
static void zlib_compress(Out_buf *o, const unsigned char *in, long n) {
  unsigned long adler;

  out_byte(o, 0x78); /* Deflate, 32K window */
  out_byte(o, 0x9C); /* Default level, no dictionary, check bits */
  deflate_fixed(o, in, n);
  adler = adler32(in, (size_t)n);
  out_byte(o, (unsigned char)(adler >> 24));
  out_byte(o, (unsigned char)(adler >> 16));
  out_byte(o, (unsigned char)(adler >> 8));
  out_byte(o, (unsigned char)adler);
}

/*====================== Scanline filtering =======================*/

/* The filter types of the PNG specification, in its numbering. */
typedef enum {
  FILTER_NONE,
  FILTER_SUB,
  FILTER_UP,
  FILTER_AVERAGE,
  FILTER_PAETH,
  FILTER_COUNT
} Png_filter;

static int paeth(int a, int b, int c) {
  int p = a + b - c;
  int pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);

  if (pa <= pb && pa <= pc) {
    return a;
  }
  return (pb <= pc) ? b : c;
}

/*
 * Filter the HEIGHT rows of ROWBYTES bytes at ROWS into OUT, which
 * receives HEIGHT * (ROWBYTES + 1) bytes: a filter type byte followed
 * by the filtered row.  BPP is the number of bytes per complete pixel
 * (rounded up to 1).  For each row the filter giving the smallest sum
 * of absolute values is used.
 */
static void filter_rows(const unsigned char *rows, int height, int rowbytes,
                        int bpp, unsigned char *out) {
  const unsigned char *row, *up;
  unsigned char *cand, *best, *dst;
  unsigned long sum, best_sum;
  Png_filter f;
  int y, i, a, b, c;

  cand = (unsigned char *)smalloc(5 * (size_t)rowbytes);
  up = NULL;
  for (y = 0; y < height; y++) {
    row = rows + (size_t)y * rowbytes;
    best = NULL;
    best_sum = 0;
    for (f = FILTER_NONE; f < FILTER_COUNT; f++) {
      dst = cand + f * rowbytes;
      for (i = 0; i < rowbytes; i++) {
        a = (i >= bpp) ? row[i - bpp] : 0;
        b = (up != NULL) ? up[i] : 0;
        c = (up != NULL && i >= bpp) ? up[i - bpp] : 0;
        switch (f) {
        case FILTER_NONE:
          dst[i] = row[i];
          break;
        case FILTER_SUB:
          dst[i] = (unsigned char)(row[i] - a);
          break;
        case FILTER_UP:
          dst[i] = (unsigned char)(row[i] - b);
          break;
        case FILTER_AVERAGE:
          dst[i] = (unsigned char)(row[i] - ((a + b) >> 1));
          break;
        default:
          dst[i] = (unsigned char)(row[i] - paeth(a, b, c));
          break;
        }
      }
      sum = 0;
      for (i = 0; i < rowbytes; i++) {
        sum += (dst[i] < 128) ? dst[i] : 256 - dst[i];
      }
      if (best == NULL || sum < best_sum) {
        best = dst;
        best_sum = sum;
      }
    }
    out[(size_t)y * (rowbytes + 1)] = (unsigned char)((best - cand) / rowbytes);
    memcpy(out + (size_t)y * (rowbytes + 1) + 1, best, rowbytes);
    up = row;
  }
  sfree(cand);
}

/*========================= PNG file ==========================*/

/* Color types, as numbered in the IHDR chunk. */
typedef enum {
  PNG_GREY = 0,
  PNG_RGB = 2,
  PNG_GREY_ALPHA = 4,
  PNG_RGBA = 6
} Png_color_type;

enum { IDAT_MAX = 1024 * 1024 }; /* Bytes of compressed data per chunk */

static void put_u32(unsigned char *p, unsigned long v) {
  p[0] = (unsigned char)(v >> 24);
  p[1] = (unsigned char)(v >> 16);
  p[2] = (unsigned char)(v >> 8);
  p[3] = (unsigned char)v;
}

static void write_chunk(FILE *file, const char *type,
                        const unsigned char *data, size_t len) {
  unsigned char buf[4];
  unsigned long crc;

  put_u32(buf, (unsigned long)len);
  fwrite(buf, 1, 4, file);
  fwrite(type, 1, 4, file);
  if (len > 0) {
    fwrite(data, 1, len, file);
  }
  crc = crc_update(0xffffffffUL, (const unsigned char *)type, 4);
  crc = crc_update(crc, data, len) ^ 0xffffffffUL;
  put_u32(buf, crc);
  fwrite(buf, 1, 4, file);
}

static void png_write(FILE *file, int width, int height, int bit_depth,
                      Png_color_type color_type, int bpp, int rowbytes,
                      const unsigned char *rows) {
  static const unsigned char signature[8] = {0x89, 'P',  'N',  'G',
                                             0x0D, 0x0A, 0x1A, 0x0A};
  unsigned char ihdr[13];
  unsigned char *filtered;
  char text[64];
  Out_buf zdata;
  size_t filtered_len, done, n;

  tables_init();

  fwrite(signature, 1, 8, file);

  put_u32(ihdr, (unsigned long)width);
  put_u32(ihdr + 4, (unsigned long)height);
  ihdr[8] = (unsigned char)bit_depth;
  ihdr[9] = (unsigned char)color_type;
  ihdr[10] = 0; /* Compression method: deflate */
  ihdr[11] = 0; /* Filter method: adaptive */
  ihdr[12] = 0; /* Not interlaced */
  write_chunk(file, "IHDR", ihdr, 13);

  /* Keyword, NUL, text: the PNG counterpart of the PPM header comment. */
  snprintf(text, sizeof(text), "Software%cSIPP %s", '\0', SIPP_VERSION);
  write_chunk(file, "tEXt", (unsigned char *)text, 9 + strlen(text + 9));

  filtered_len = (size_t)height * (rowbytes + 1);
  filtered = (unsigned char *)smalloc(filtered_len > 0 ? filtered_len : 1);
  filter_rows(rows, height, rowbytes, bpp, filtered);

  out_init(&zdata, filtered_len / 2 + 64);
  zlib_compress(&zdata, filtered, (long)filtered_len);
  sfree(filtered);

  for (done = 0; done < zdata.len; done += n) {
    n = zdata.len - done;
    if (n > IDAT_MAX) {
      n = IDAT_MAX;
    }
    write_chunk(file, "IDAT", zdata.data + done, n);
  }
  sfree(zdata.data);

  write_chunk(file, "IEND", NULL, 0);
  fflush(file);
}

void sipp_png_write(FILE *file, int width, int height, int channels,
                    const unsigned char *rows) {
  static const Png_color_type color_type[5] = {PNG_GREY, PNG_GREY,
                                               PNG_GREY_ALPHA, PNG_RGB,
                                               PNG_RGBA};

  if (channels < 1 || channels > 4) {
    fprintf(stderr, "sipp_png_write: %d channels not supported\n", channels);
    return;
  }
  png_write(file, width, height, 8, color_type[channels], channels,
            width * channels, rows);
}

void sipp_png_write_bitmap(FILE *file, int width, int height,
                           const unsigned char *rows) {
  unsigned char *inverted;
  size_t nbytes, i;
  int rowbytes;

  /* PNG has 0 = black in a 1 bit greyscale image; bitmaps have 1. */
  rowbytes = (width + 7) / 8;
  nbytes = (size_t)rowbytes * height;
  inverted = (unsigned char *)smalloc(nbytes > 0 ? nbytes : 1);
  for (i = 0; i < nbytes; i++) {
    inverted[i] = (unsigned char)~rows[i];
  }
  png_write(file, width, height, 1, PNG_GREY, 1, rowbytes, inverted);
  sfree(inverted);
}
