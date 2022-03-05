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

#include <malloc.h>

#define sfree(x) free(x)

extern void *
smalloc _ANSI_ARGS_((int size));

extern void *
scalloc _ANSI_ARGS_((int size,
                     int itemsize));

extern void *
srealloc _ANSI_ARGS_((void *ptr,
                      int   size));


#endif /* SMALLOC_H */
