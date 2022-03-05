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
 ** bezier.h - Types and defines needed by bezier.c
 **/

#ifndef BEZIER_H
#define BEZIER_H

#include <geometric.h>


#define PATCHES     1
#define CURVES      2
#define NVERTICES   3
#define NPATCHES    4
#define NCURVES     5
#define VERTEX_LIST 6
#define PATCH_LIST  7
#define CURVE_LIST  8
#define INTEGER     9
#define FLOAT       10


typedef union {
    int    intval;
    double floatval;
} Tokenval;


typedef struct {
    int cp[4];
} Bez_Curve;

typedef struct {
    int cp[4][4];
} Bez_Patch;

typedef struct {
    int         type;
    int         nvertex;
    Vector *vertex;
    union {
        int ncurves;
        int npatches;
    } n;
    union {
        Bez_Curve *ccp;
        Bez_Patch *pcp;
    } cp;
} Bez_Object;


#endif /* BEZIER_H */
