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
 ** bezier.h - Types and defines needed by bezier.c
 **/

#ifndef BEZIER_H
#define BEZIER_H

#include <geometric.h>

/*
 * The tokens of a bezier description file.
 */
typedef enum {
  END_OF_INPUT = 0,
  PATCHES,
  CURVES,
  NVERTICES,
  NPATCHES,
  NCURVES,
  VERTEX_LIST,
  PATCH_LIST,
  CURVE_LIST,
  INTEGER,
  FLOAT
} Bezier_token;

typedef union {
  int intval;
  double floatval;
} Tokenval;

/*
 * The tokenizer (bezier_lex.c).
 */
EXTERN void bezier_lex_open(FILE *file);
EXTERN Bezier_token bezier_lex(void);
EXTERN void bezier_lex_close(void);

typedef struct {
  int cp[4];
} Bez_Curve;

typedef struct {
  int cp[4][4];
} Bez_Patch;

typedef struct {
  Bezier_token type; /* PATCHES or CURVES */
  int nvertex;
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
