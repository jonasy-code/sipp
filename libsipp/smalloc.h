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
 ** smalloc.h - Interface to smalloc.c
 **/

#ifndef SMALLOC_H
#define SMALLOC_H

#include <stdlib.h>

#define sfree(x) free(x)

extern void *smalloc(int size);

extern void *scalloc(int size, int itemsize);

extern void *srealloc(void *ptr, int size);

#endif /* SMALLOC_H */
