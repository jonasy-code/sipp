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
 ** smalloc.c - "Safe" malloc and calloc.
 **/

#include <stdlib.h>
#include <stdio.h>

#include <sipp.h>
#include <smalloc.h>


void *
smalloc(int size)
{
    char *p;

    p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "smalloc(): Out of memory.\n");
        exit(1);
    }

    return p;
}


void *
scalloc(int size, int itemsize)
{
    char *p;

    p = (char *)calloc(size, itemsize);
    if (p == NULL) {
        fprintf(stderr, "scalloc(): Out of memory.\n");
        exit(1);
    }

    return p;
}


void *
srealloc(void *ptr, int size)
{
    char *p;
 
    p = realloc(ptr, size);
    if (p == NULL) {
        fprintf(stderr, "srealloc(): Out of memory.\n");
        exit(1);
    }

    return p;
}

