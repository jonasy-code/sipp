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
 ** objects.h - Types and interface to objects.c
 **/

#ifndef OBJECT_H
#define OBJECT_H

#include <sipp.h>

/*
 * Structure to keep a list of vertex references.
 */
typedef struct vertex_ref_t {
  Vertex *vertex;
  struct vertex_ref_t *next;
} Vertex_ref;

extern void objects_init(void);

#endif /* OBJECT_H */
