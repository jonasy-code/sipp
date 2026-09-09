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
 ** sipp_bitmap.c - A sample bitmap for use in SIPP and routines to use it.
 **/

#include <sys/types.h>
#include <stdio.h>

#include <sipp.h>
#include <smalloc.h>
#include <sipp_bitmap.h>


/* ================================================================ */


/*
 * Return a pointer to a new Sipp_bitmap, with the size WIDTH x HEIGHT.
 * Enough space is allocated so each line ends on a byte boundary.
 */
Sipp_bitmap *
sipp_bitmap_create(int width, int height)
{
    Sipp_bitmap  * bm;

    bm = (Sipp_bitmap *) smalloc(sizeof(Sipp_bitmap));
    bm->width = width;
    bm->height = height;
    bm->width_bytes = (width >> 3);
    if ((width & 7) != 0) {
        bm->width_bytes++;
    } 
    bm->buffer = (unsigned char *) scalloc(bm->width_bytes * height, sizeof(unsigned char));

    return bm;
}



/*
 * Destruct a bitmap, and free allocated memory.
 */
void
sipp_bitmap_destruct(Sipp_bitmap * bm)
{
    if (bm != NULL) {
        if (bm->buffer != NULL) {
            sfree(bm->buffer);
        }
        sfree(bm);
    }
}




/*
 * Draw a line in the Sipp_bitmap BM from (X1, Y1)
 * to (X2, Y2)
 */

#define PLOT(bm, width_bytes, yres, x, y) \
        (bm)[(y) * (width_bytes) + ((x) >> 3)] |= (1 << (7 - ((x) & 7)))

void
sipp_bitmap_line(Sipp_bitmap * bm, int x1, int y1, int x2, int y2)
{
    int   d;
    int   x,  y;
    int   ax, ay;
    int   sx, sy;
    int   dx, dy;

    dx = x2 - x1;
    ax = abs(dx) << 1;
    sx = ((dx < 0) ? -1 : 1);
    dy = y2 - y1;
    ay = abs(dy) << 1;
    sy = ((dy < 0) ? -1 : 1);

    x = x1;
    y = y1;
    if (ax > ay) {
        d = ay - (ax >> 1);
        for (;;) {
            PLOT(bm->buffer, bm->width_bytes, bm->height, x, y);
            if (x == x2) {
                return;
            }
            if (d >= 0) {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    } else {
        d = ax - (ay >> 1);
        for (;;) {
            PLOT(bm->buffer, bm->width_bytes, bm->height, x, y);
            if (y == y2) {
                return;
            }
            if (d >= 0) {
                x += sx;
                d -=  ay;
            }
            y += sy;
            d += ax;
        }
    }
}



/*
 * Write the Sipp_bitmap BM to the open file FILE.
 */

void
sipp_bitmap_write(FILE * file, Sipp_bitmap * bm)
{
    int    written;
    int    wrote;
    int    left;

    fprintf(file,  "P4\n");
    fprintf(file,  "#Image rendered with SIPP %s\n",  SIPP_VERSION);
    fprintf(file,  "%d\n%d\n", bm->width, bm->height);

    wrote   = 0;
    written = 0;
    left    = bm->width_bytes * bm->height;
    while ((wrote = fwrite(bm->buffer + written, 1, left, file)) != left) {
        written += wrote;
        left -= wrote;
    }
}
