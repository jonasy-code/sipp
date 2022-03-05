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
 ** lightsource.h - Interface to lightsource.c
 **/

#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H

#include <sipp.h>


/*
 * Information needed in a directional lightsource.
 */
typedef struct {
    Vector dir;
} Dir_light_info;

/*
 * Information needed in a spotlight.
 */
typedef struct {
    Vector      pos;
    Vector      point;
    Vector      dir;
    double      cos_fov;
} Spot_light_info;


extern Lightsource  *lightsrc_stack;  /* Lightsource list. */


#define lightsource_init()    lightsrc_stack = NULL

extern void
depthmaps_create _ANSI_ARGS_((void));

extern void
depthmaps_destruct _ANSI_ARGS_((void));


#endif /* LIGHTSOURCE_H */
