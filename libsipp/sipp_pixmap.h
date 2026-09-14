/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright 1992 Jonas Yngvesson, Inge Wallin
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
 ** sipp_pixmap.h - Interface to sipp_pixmap.c.
 **/

#ifndef SIPP_PIXMAP_H
#define SIPP_PIXMAP_H

#include <sys/types.h>

/* The generic pixel setter usable for any pixmap type. */
typedef void (*Pixmap_set_pixel_func)(void *pm, int x, int y, unsigned char red,
                                      unsigned char grn, unsigned char blu,
                                      unsigned char alpha);

/* The SIPP pixmap and its associated functions. */

typedef struct {
  int width;
  int height;
  unsigned char *buffer;
} Sipp_pixmap;

EXTERN Sipp_pixmap *sipp_pixmap_create(int width, int height);

EXTERN void sipp_pixmap_destruct(Sipp_pixmap *pm);

EXTERN void sipp_pixmap_set_pixel(Sipp_pixmap *pm, int x, int y,
                                  unsigned char red, unsigned char grn,
                                  unsigned char blu, unsigned char alpha);

EXTERN void sipp_pixmap_write(FILE *file, Sipp_pixmap *pm);

EXTERN void sipp_pixmap_write_png(FILE *file, Sipp_pixmap *pm);

#endif /* SIPP_PIXMAP_H */
