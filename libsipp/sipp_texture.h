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
 ** sipp_texture.h - Image textures, filtered for shaders.
 **
 ** A Sipp_texture is an image with 1 to 4 channels of 8 bits, kept
 ** together with a pyramid of half-size copies (a mipmap), so that a
 ** shader can look it up filtered over the area its sample covers:
 ** the level of the pyramid whose texels are about the size of the
 ** sample is interpolated in (trilinear filtering), and an elongated
 ** sample is averaged over several points along its long axis
 ** (anisotropic filtering).  Textures are read-only once created, so
 ** any number of threads may sample one at the same time.
 **/

#ifndef SIPP_TEXTURE_H
#define SIPP_TEXTURE_H

#include <sipp.h>
#include <sipp_bitmap.h>
#include <sipp_pixmap.h>

typedef struct {
  int channels;          /* Values per texel, 1 to 4 */
  int levels;            /* Levels in the pyramid, level 0 the full one */
  int *width;            /* Size of each level ... */
  int *height;           /* ... in texels */
  unsigned char **texel; /* The texels of each level, rows top first */
  bool wrap;             /* Repeat the image outside [0, 1] (the
                            default), rather than clamp to the edge */
} Sipp_texture;

/*
 * Create a texture from HEIGHT rows of WIDTH texels of CHANNELS bytes
 * each, top row first, without padding (the layout of a Sipp_pixmap
 * with 3 channels).  The pixels are copied.
 */
EXTERN Sipp_texture *sipp_texture_create(int width, int height, int channels,
                                         const unsigned char *pixels);

/* A 3 channel texture from a pixmap. */
EXTERN Sipp_texture *sipp_texture_from_pixmap(const Sipp_pixmap *pm);

/* A 1 channel texture from a bitmap: 1.0 where it is white (bit 0). */
EXTERN Sipp_texture *sipp_texture_from_bitmap(const Sipp_bitmap *bm);

EXTERN void sipp_texture_destruct(Sipp_texture *tex);

/*
 * Look up the texture at (U, V), with (0, 0) the upper left corner of
 * the image and (1, 1) the lower right, filtered over the parallelogram
 * spanned by the two vectors (DUDX, DVDX) and (DUDY, DVDY): the change
 * of (u, v) over one sample in the x and y directions of the image.
 * VALUES receives one value in [0, 1] per channel.  All zero
 * derivatives give a bilinear lookup in the full-size image.
 */
EXTERN void sipp_texture_sample(const Sipp_texture *tex, double u, double v,
                                double dudx, double dvdx, double dudy,
                                double dvdy, double *values);

/*
 * The same, with (u, v) the first two texture coordinates of SP and
 * the derivatives taken from it.
 */
EXTERN void sipp_texture_lookup(const Sipp_texture *tex, const Shade_point *sp,
                                double *values);

#endif /* SIPP_TEXTURE_H */
