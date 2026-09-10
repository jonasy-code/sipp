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
 ** pixel.h - Interface to pixel.c
 **/

#ifndef _PIXEL_H
#define _PIXEL_H

#include <rendering.h>
#include <sipp.h>

extern Color sipp_bgcol;

/*
 * Entry in a position in the pixel buffer.
 */
typedef struct {
  Active_edge *edge;
  Vector worldstep;
  Vector texturestep;
  Vector normalstep;
  double offset;
  double hden;
  double depth;
  int next;
} Pixel_info;

/*
 * Shading cache entry, see pixel_collect().
 */
typedef struct {
  int polygon;
  Color color;
  Color opacity;
} Shade_entry;

/*
 * A pixel buffer: the fragments of one scanline, and the shading
 * cache.  Each scanline sweep owns one.
 */
typedef struct {
  Pixel_info *pixbuf;       /* The fragments */
  int pixbuf_size;          /* Current size of pixbuf */
  int size_delta;           /* How much to grow it each time */
  int first_free;           /* First unused entry */
  Shade_entry *shade_cache; /* shade_npixels * SHADE_SLOTS entries */
  int *shade_count;         /* Entries in use, per output pixel */
  int shade_npixels;
} Pixel_buffer;

enum { SHADE_SLOTS = 8 };

extern void pixels_setup(Pixel_buffer *pb, int init_size);

extern void pixels_free(Pixel_buffer *pb);

extern void pixels_reinit(Pixel_buffer *pb);

extern int pixel_insert(Pixel_buffer *pb, int pixel, Vector *worldstep,
                        Vector *texturestep, Vector *normalstep, double depth,
                        double hden, double offset, Active_edge *edge);

extern void pixel_collect(Pixel_buffer *pb, int pixel, Color *color,
                          Color *opacity, Render_mode render_mode,
                          int cache_slot);

extern void shade_cache_setup(Pixel_buffer *pb, int npixels);

extern void shade_cache_clear(Pixel_buffer *pb);

extern void shade_cache_free(Pixel_buffer *pb);

#endif
