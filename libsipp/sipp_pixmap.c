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
 ** sipp_pixmap.c - A sample pixmap for use in SIPP and routines to use it.
 **/

#include <stdio.h>
#include <sys/types.h>

#include <sipp.h>
#include <sipp_pixmap.h>
#include <sipp_png.h>
#include <smalloc.h>

/* ================================================================ */

/*
 * Return a pointer to a new Sipp_pixmap, with the size WIDTH x HEIGHT.
 */

Sipp_pixmap *sipp_pixmap_create(int width, int height) {
  Sipp_pixmap *pm;

  pm = (Sipp_pixmap *)smalloc(sizeof(Sipp_pixmap));
  pm->width = width;
  pm->height = height;
  pm->buffer =
      (unsigned char *)scalloc(3 * width * height, sizeof(unsigned char));

  return pm;
}

/*
 * Destruct a pixmap, and free allocated memory.
 */

void sipp_pixmap_destruct(Sipp_pixmap *pm) {
  if (pm != NULL) {
    if (pm->buffer != NULL) {
      sfree(pm->buffer);
    }
    sfree(pm);
  }
}

/*
 * Set the pixel at (X, Y) in Sipp_pixmap PM to the color
 * (RED, GRN, BLU).  ALPHA is accepted so that this function can be
 * used as a Pixel_func, but the pixmap has no alpha channel and it is
 * ignored.
 */

void sipp_pixmap_set_pixel(Sipp_pixmap *pm, int x, int y, unsigned char red,
                           unsigned char grn, unsigned char blu,
                           unsigned char alpha) {
  unsigned char *cp;

  if (x < 0 || y < 0 || x >= pm->width || y >= pm->height) {
    fprintf(stderr,
            "Tried to write to pixel (%d, %d) in a Sipp_pixmap which was only "
            "%d x %d.\n",
            x, y, pm->width, pm->height);
  } else {
    cp = pm->buffer + 3 * (y * pm->width + x);
    *cp++ = red;
    *cp++ = grn;
    *cp = blu;
  }
}

/*
 * Write the Sipp_pixmap PM to the open file FILE.
 */

void sipp_pixmap_write(FILE *file, Sipp_pixmap *pm) {
  unsigned char *byte;
  int nbytes;
  int i;

  fprintf(file, "P6\n");
  fprintf(file, "#Image rendered with SIPP %s\n", SIPP_VERSION);
  fprintf(file, "%d\n%d\n255\n", pm->width, pm->height);

  byte = pm->buffer;
  nbytes = pm->width * pm->height;
  for (i = 0; i < nbytes; ++i) {
    putc(*byte++, file);
    putc(*byte++, file);
    putc(*byte++, file);
  }
}

/*
 * Write the pixmap PM as an RGB PNG file.
 */
void sipp_pixmap_write_png(FILE *file, Sipp_pixmap *pm) {
  sipp_png_write(file, pm->width, pm->height, 3, pm->buffer);
}
