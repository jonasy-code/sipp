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
 ** rendering.h - Types and interface to the rendering.c
 **/

#ifndef RENDERING_H
#define RENDERING_H

#include <sipp.h>
#include <geometric.h>

/*
 * Modes for storing the image.
 */
#define PBM_FILE   0
#define PPM_FILE   1
#define FUNCTION   2


/*
 * Temporary storage of transformed vertices.
 */
typedef struct view_coord_3d {
    Vector                view;     /* Transformed view coordinates */
    double                hden;      /* Homogenous denominator */
    Vector                world;     /* Transformed world voordinates */
    Vector                normal;    /* average normal */
    Vector                texture;   /* texture parameters */
    struct view_coord_3d *next;      /* next vertex in the list */
} View_coord;


/*
 * Entry in the edge list used in rendering.
 */
typedef struct edges_3d {
    int              ystart;
    int              ystop;
    double           xstart;
    double           xstep;
    double           hden;
    double           hdenstep;
    Vector           world;       /* World coordinates */
    Vector           worldstep;   /* World coordinates interpolation steps */
    Vector           normal;      /* Surface normal */
    Vector           normalstep;  /* Surface normal interpolation steps */
    Vector           texture;     /* Texture coordinates */
    Vector           texturestep; /* Texture coordinates interpolation steps */
    int              polygon;     /* Id of the polygon the edge belongs to */
    Surface         *surface;     /* Surface that the edge belongs to */
    struct edges_3d *next;        /* Next edge on this scanline */
} Edge;


#ifdef SAVE_MEMORY

typedef struct surf_box_t {
    Surface           *surface;
    Transf_mat         mat;
    struct surf_box_t *next
} Surface_box;

typedef struct {
    Edge        *edge;
    Surface_box *surf_box;
} Bucket_entry;

#endif 

#endif /* RENDERING_H */
