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
 ** sipp_png.h - Writing images as PNG files.
 **/

#ifndef SIPP_PNG_H
#define SIPP_PNG_H

#include <stdio.h>

#include <sipp.h>

/*
 * Write an 8 bit per channel image as a PNG file.  CHANNELS is 1 (grey),
 * 2 (grey + alpha), 3 (RGB) or 4 (RGB + alpha).  ROWS points at HEIGHT
 * rows of WIDTH * CHANNELS bytes each, top row first, without padding.
 * Alpha is straight (not premultiplied), as PNG requires.
 */
EXTERN void sipp_png_write(FILE *file, int width, int height, int channels,
                           const unsigned char *rows);

/*
 * Write a 1 bit per pixel image as a PNG file.  ROWS points at HEIGHT
 * rows of (WIDTH + 7) / 8 bytes, top row first, eight pixels per byte
 * with the leftmost in the most significant bit, and 1 meaning black:
 * the layout of a Sipp_bitmap and of a raw PBM file.
 */
EXTERN void sipp_png_write_bitmap(FILE *file, int width, int height,
                                  const unsigned char *rows);

#endif /* SIPP_PNG_H */
