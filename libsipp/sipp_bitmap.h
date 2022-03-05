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
 ** sipp_bitmap.h - Interface to sipp_bitmap.c
 **/

#ifndef BITMAP_H
#define BITMAP_H

#include <sys/types.h>

/* The generic line drawer usable for any bitmap type. */
typedef void   (*Bitmap_line_func)();


/* The SIPP bitmap and its associated functions. */

typedef struct{
    int       width;
    int       height;
    int       width_bytes;
    u_char   *buffer;
} Sipp_bitmap;


EXTERN Sipp_bitmap *
sipp_bitmap_create _ANSI_ARGS_((int   width,
                                int   height));

EXTERN void
sipp_bitmap_destruct _ANSI_ARGS_((Sipp_bitmap  *bm));

EXTERN void
sipp_bitmap_line _ANSI_ARGS_((Sipp_bitmap  *bm,
                              int           x1,
                              int           y1,
                              int           x2,
                              int           y2));

EXTERN void
sipp_bitmap_write _ANSI_ARGS_((FILE         *file,
                               Sipp_bitmap  *bm));


#endif /* BITMAP_H */
