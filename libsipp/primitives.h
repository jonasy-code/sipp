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
 ** primitives.h - Interface to the various primitive object functions.
 **/

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <sipp.h>

/*
 * Types of texture coordinates.
 */
#define WORLD 0
#define CYLINDRICAL 1
#define SPHERICAL 2
#define NATURAL 3

EXTERN Object *sipp_torus(double bigradius, double smallradius, int res1,
                          int res2, void *surface, Shader *shader, int texture);

EXTERN Object *sipp_cone(double radius_bot, double radius_top, double length,
                         int res, void *surface, Shader *shader, int texture);

EXTERN Object *sipp_cylinder(double radius, double length, int res,
                             void *surface, Shader *shader, int texture);

EXTERN Object *sipp_ellipsoid(double x_rad, double y_rad, double z_rad, int res,
                              void *surface, Shader *shader, int texture);

EXTERN Object *sipp_sphere(double radius, int res, void *surface,
                           Shader *shader, int texture);

EXTERN Object *sipp_prism(int num_points, Vector *points, double length,
                          void *surface, Shader *shader, int texture);

EXTERN Object *sipp_block(double xsize, double ysize, double zsize,
                          void *surface, Shader *shader, int texture);

EXTERN Object *sipp_cube(double size, void *surface, Shader *shader,
                         int texture);

EXTERN Object *sipp_bezier_file(FILE *file, int res, void *surface,
                                Shader *shader, int texture);

EXTERN Object *sipp_bezier_patches(int nvert, Vector *vertex, int npatch,
                                   int *cp_index, int res, void *surface,
                                   Shader *shader, int texture);

EXTERN Object *sipp_bezier_rotcurve(int nvert, Vector *vertex, int ncurve,
                                    int *cp_index, int res, void *surface,
                                    Shader *shader, int texture);

EXTERN Object *sipp_teapot(int resolution, void *surface, Shader *shader,
                           int texture);

EXTERN Object *sipp_teapot_body(int resolution, void *surface, Shader *shader,
                                int texture);

EXTERN Object *sipp_teapot_lid(int resolution, void *surface, Shader *shader,
                               int texture);

EXTERN Object *sipp_teapot_spout(int resolution, void *surface, Shader *shader,
                                 int texture);

EXTERN Object *sipp_teapot_handle(int resolution, void *surface, Shader *shader,
                                  int texture);

#endif /* PRIMITIVES_H */
