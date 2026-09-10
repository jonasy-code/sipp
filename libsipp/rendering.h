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

#include <geometric.h>
#include <sipp.h>

/*
 * Modes for storing the image.
 */
#define PBM_FILE 0
#define PPM_FILE 1
#define FUNCTION 2

/*
 * Temporary storage of transformed vertices.
 */
typedef struct view_coord_3d {
  Vector view;                /* Transformed view coordinates */
  double hden;                /* Homogenous denominator */
  Vector world;               /* Transformed world voordinates */
  Vector normal;              /* average normal */
  Vector texture;             /* texture parameters */
  struct view_coord_3d *next; /* next vertex in the list */
} View_coord;

/*
 * Entry in the edge list used in rendering.
 */
/*
 * An edge of a polygon, as created by the object tree traversal.  Edges
 * are bucketed by their first scanline (y_bucket) and are never
 * modified by the scanline sweep, so several sweeps (bands, threads)
 * can share them.
 */
typedef struct edge_t {
  int ystart;      /* First scanline */
  int ystop;       /* Last scanline */
  double xstart;   /* x at ystart ... */
  double xstep;    /* ... and its change per scanline */
  double hden;     /* 1/w at ystart, for perspective correct */
  double hdenstep; /* interpolation */
  Vector world;    /* World position at ystart */
  Vector worldstep;
  Vector normal; /* Normal at ystart */
  Vector normalstep;
  Vector texture; /* Texture coordinates at ystart */
  Vector texturestep;
  int polygon;            /* Id of the polygon the edge belongs to */
  Surface *surface;       /* Surface that the edge belongs to */
  int id;                 /* Index of the edge, unique in a pass */
  struct edge_t *next;    /* Next edge in the same y_bucket */
  struct edge_t *sibling; /* Ring of all edges of the same polygon */
} Edge;

/*
 * The state of an edge while it crosses the current scanline of a
 * sweep: the interpolated values, stepped from scanline to scanline,
 * and the links of the active list.
 */
typedef struct active_edge_t {
  Edge *edge;
  int y;       /* Current scanline */
  double x;    /* Current x */
  double hden; /* Current 1/w */
  Vector world;
  Vector normal;
  Vector texture;
  struct active_edge_t *next;        /* Next/previous edge in the active */
  struct active_edge_t *prev;        /* list */
  struct active_edge_t *retire_next; /* Retirement bucket chain, used
                                        while a band replays */
} Active_edge;

#ifdef SAVE_MEMORY

typedef struct surf_box_t {
  Surface *surface;
  Transf_mat mat;
  struct surf_box_t *next
} Surface_box;

typedef struct {
  Edge *edge;
  Surface_box *surf_box;
} Bucket_entry;

#endif

#endif /* RENDERING_H */
