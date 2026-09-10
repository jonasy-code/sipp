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
 ** sipp_texture.c - Image textures with mipmaps.  See sipp_texture.h.
 **/

#include <math.h>
#include <string.h>

#include <sipp.h>

#include <sipp_texture.h>
#include <smalloc.h>

enum {
  MAX_CHANNELS = 4,
  MAX_TAPS = 8 /* Points along the long axis of a sample */
};

/*
 * Level I + 1 of the pyramid from level I: every texel is the average
 * of a 2 x 2 block.  An odd size is rounded up, the last block then
 * reading the edge texel twice.
 */
static void downsample(const Sipp_texture *tex, int i) {
  int w = tex->width[i], h = tex->height[i];
  int w2 = tex->width[i + 1], h2 = tex->height[i + 1];
  int ch = tex->channels;
  const unsigned char *src = tex->texel[i];
  unsigned char *dst = tex->texel[i + 1];
  int x, y, c, x0, x1, y0, y1;

  for (y = 0; y < h2; y++) {
    y0 = 2 * y;
    y1 = (y0 + 1 < h) ? y0 + 1 : y0;
    for (x = 0; x < w2; x++) {
      x0 = 2 * x;
      x1 = (x0 + 1 < w) ? x0 + 1 : x0;
      for (c = 0; c < ch; c++) {
        dst[(y * w2 + x) * ch + c] =
            (unsigned char)((src[(y0 * w + x0) * ch + c] +
                             src[(y0 * w + x1) * ch + c] +
                             src[(y1 * w + x0) * ch + c] +
                             src[(y1 * w + x1) * ch + c] + 2) /
                            4);
      }
    }
  }
}

Sipp_texture *sipp_texture_create(int width, int height, int channels,
                                  const unsigned char *pixels) {
  Sipp_texture *tex;
  int i, w, h;

  if (width < 1 || height < 1 || channels < 1 || channels > MAX_CHANNELS) {
    return NULL;
  }
  tex = (Sipp_texture *)smalloc(sizeof(Sipp_texture));
  tex->channels = channels;
  tex->wrap = TRUE;
  tex->levels = 1;
  for (w = width, h = height; w > 1 || h > 1;
       w = (w + 1) / 2, h = (h + 1) / 2) {
    tex->levels++;
  }
  tex->width = (int *)smalloc(tex->levels * sizeof(int));
  tex->height = (int *)smalloc(tex->levels * sizeof(int));
  tex->texel = (unsigned char **)smalloc(tex->levels * sizeof(unsigned char *));
  for (i = 0, w = width, h = height; i < tex->levels;
       i++, w = (w + 1) / 2, h = (h + 1) / 2) {
    tex->width[i] = w;
    tex->height[i] = h;
    tex->texel[i] = (unsigned char *)smalloc((size_t)w * h * channels);
  }
  memcpy(tex->texel[0], pixels, (size_t)width * height * channels);
  for (i = 0; i + 1 < tex->levels; i++) {
    downsample(tex, i);
  }
  return tex;
}

Sipp_texture *sipp_texture_from_pixmap(const Sipp_pixmap *pm) {
  return sipp_texture_create(pm->width, pm->height, 3, pm->buffer);
}

Sipp_texture *sipp_texture_from_bitmap(const Sipp_bitmap *bm) {
  Sipp_texture *tex;
  unsigned char *grey;
  int x, y;

  grey = (unsigned char *)smalloc((size_t)bm->width * bm->height);
  for (y = 0; y < bm->height; y++) {
    for (x = 0; x < bm->width; x++) {
      grey[y * bm->width + x] =
          (bm->buffer[y * bm->width_bytes + x / 8] & (0x80 >> (x % 8))) ? 0
                                                                        : 255;
    }
  }
  tex = sipp_texture_create(bm->width, bm->height, 1, grey);
  sfree(grey);
  return tex;
}

void sipp_texture_destruct(Sipp_texture *tex) {
  int i;

  if (tex == NULL) {
    return;
  }
  for (i = 0; i < tex->levels; i++) {
    sfree(tex->texel[i]);
  }
  sfree(tex->texel);
  sfree(tex->width);
  sfree(tex->height);
  sfree(tex);
}

/*
 * A texel index along one axis of a level N texels long, wrapped or
 * clamped into range.
 */
static int wrap_index(int i, int n, bool wrap) {
  if (wrap) {
    i %= n;
    return (i < 0) ? i + n : i;
  }
  return (i < 0) ? 0 : (i >= n) ? n - 1 : i;
}

/*
 * Bilinear interpolation in level LEVEL at the texel coordinates
 * (X, Y), where texel centers are at half-integers.  Adds the values
 * times WEIGHT into SUM.
 */
static void sample_level(const Sipp_texture *tex, int level, double x, double y,
                         double weight, double *sum) {
  int w = tex->width[level], h = tex->height[level], ch = tex->channels;
  const unsigned char *t = tex->texel[level];
  double fx, fy;
  int x0, y0, x1, y1, c;
  double w00, w01, w10, w11;

  x -= 0.5;
  y -= 0.5;
  fx = floor(x);
  fy = floor(y);
  x0 = wrap_index((int)fx, w, tex->wrap);
  y0 = wrap_index((int)fy, h, tex->wrap);
  x1 = wrap_index((int)fx + 1, w, tex->wrap);
  y1 = wrap_index((int)fy + 1, h, tex->wrap);
  fx = x - fx;
  fy = y - fy;
  w00 = (1.0 - fx) * (1.0 - fy) * weight;
  w10 = fx * (1.0 - fy) * weight;
  w01 = (1.0 - fx) * fy * weight;
  w11 = fx * fy * weight;
  for (c = 0; c < ch; c++) {
    sum[c] += w00 * t[(y0 * w + x0) * ch + c] +
              w10 * t[(y0 * w + x1) * ch + c] +
              w01 * t[(y1 * w + x0) * ch + c] + w11 * t[(y1 * w + x1) * ch + c];
  }
}

void sipp_texture_sample(const Sipp_texture *tex, double u, double v,
                         double dudx, double dvdx, double dudy, double dvdy,
                         double *values) {
  double w0 = tex->width[0], h0 = tex->height[0];
  double ax, ay, bx, by; /* The sample's axes, in texels */
  double A, B, C, disc, l1, l2;
  double major, minor, e0, e1, len;
  double mx, my; /* Unit vector along the major axis */
  double level, frac, width, scale, t, x, y;
  double sum[MAX_CHANNELS];
  int lo, hi, ntaps, k, c;

  /*
   * The footprint is the ellipse with the Gram matrix of the two axis
   * vectors; its half axes are the square roots of the eigenvalues.
   */
  ax = dudx * w0;
  ay = dvdx * h0;
  bx = dudy * w0;
  by = dvdy * h0;
  A = ax * ax + ay * ay;
  B = ax * bx + ay * by;
  C = bx * bx + by * by;
  disc = sqrt((A - C) * (A - C) + 4.0 * B * B);
  l1 = 0.5 * (A + C + disc);
  l2 = 0.5 * (A + C - disc);
  major = sqrt(l1 > 0.0 ? l1 : 0.0);
  minor = sqrt(l2 > 0.0 ? l2 : 0.0);

  /* The major axis in texels: the image of the eigenvector of l1. */
  if (fabs(B) > 1e-12 || A != C) {
    if (fabs(l1 - C) > fabs(l1 - A)) {
      e0 = l1 - C;
      e1 = B;
    } else {
      e0 = B;
      e1 = l1 - A;
    }
  } else {
    e0 = 1.0;
    e1 = 0.0;
  }
  mx = e0 * ax + e1 * bx;
  my = e0 * ay + e1 * by;
  len = sqrt(mx * mx + my * my);
  if (len > 0.0) {
    mx /= len;
    my /= len;
  }

  /*
   * Points along the major axis, each filtered to the minor axis (or
   * wider, if the ellipse is too elongated for MAX_TAPS points).
   */
  if (minor < 1e-6 || major < 1e-6) {
    ntaps = 1;
    width = major;
  } else {
    ntaps = (int)ceil(major / minor);
    if (ntaps > MAX_TAPS) {
      ntaps = MAX_TAPS;
    }
    width = major / ntaps;
    if (width < minor) {
      width = minor;
    }
  }

  /* The level whose texels are about WIDTH level-0 texels large. */
  level = (width > 1.0) ? log2(width) : 0.0;
  if (level > tex->levels - 1) {
    level = tex->levels - 1;
  }
  lo = (int)floor(level);
  hi = (lo + 1 < tex->levels) ? lo + 1 : lo;
  frac = level - lo;

  for (c = 0; c < tex->channels; c++) {
    sum[c] = 0.0;
  }
  for (k = 0; k < ntaps; k++) {
    t = ((k + 0.5) / ntaps - 0.5) * major;
    x = u * w0 + t * mx;
    y = v * h0 + t * my;
    scale = 1.0 / (double)(1 << lo);
    sample_level(tex, lo, x * scale, y * scale, (1.0 - frac) / ntaps, sum);
    if (frac > 0.0) {
      scale = 1.0 / (double)(1 << hi);
      sample_level(tex, hi, x * scale, y * scale, frac / ntaps, sum);
    }
  }
  for (c = 0; c < tex->channels; c++) {
    values[c] = sum[c] / 255.0;
  }
}

void sipp_texture_lookup(const Sipp_texture *tex, const Shade_point *sp,
                         double *values) {
  sipp_texture_sample(tex, sp->texture.x, sp->texture.y, sp->dtdx.x, sp->dtdx.y,
                      sp->dtdy.x, sp->dtdy.y, values);
}
