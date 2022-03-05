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
 ** ellipsoid.c - Creating ellipsiods and spheres as sipp objects.
 **/

#include <xalloca.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>


Object *
sipp_ellipsoid(x_rad, y_rad, z_rad, res, surface, shader, texture)
    double  x_rad;
    double  y_rad;
    double  z_rad;
    int     res;
    void   *surface;
    Shader *shader;
    int     texture;
{
    int      i, j;
    double   factor;
    double   factor1;
    double   factor2;
    double   zradprim;
    double   zradprim1;
    double   zradprim2;
    double  *x_arr;
    double  *y_arr;
    double  *u_arr;
    double  *v1, *v2;
    Object  *ellipsoid;
    bool     old_user_refs;
    
    /* Odd resolutions make ugly spheres since the poles will be */
    /* different in size. */
    if (res & 1) {
        res++;
    }

    /* Create two arrays with the coordinates of the points */
    /* around the perimeter at Z = 0 */
    x_arr = (double *) alloca((res + 1) * sizeof(double));
    y_arr = (double *) alloca((res + 1) * sizeof(double));
    u_arr = (double *) alloca((res + 1) * sizeof(double));
    if (texture == SPHERICAL || texture == NATURAL) {
        v1    = (double *) alloca((res + 1) * sizeof(double));
        v2    = (double *) alloca((res + 1) * sizeof(double));
    }
    for (i = 0; i <= res; i++) {
        x_arr[i] = x_rad * cos(i * 2.0 * M_PI / res);
        y_arr[i] = y_rad * sin(i * 2.0 * M_PI / res);
        u_arr[i] = (double)i / (double)res;
    }

    /* Create the top pole */
    factor = sin(2.0 * M_PI / res);
    zradprim = z_rad * cos(2.0 * M_PI / res);
    if (texture == SPHERICAL || texture == NATURAL) {
        for (i = 0; i < res; i++) {
            v2[i] = (atan(zradprim 
                          / sqrt(factor * factor * (x_arr[i] * x_arr[i] 
                                                    + y_arr[i] * y_arr[i])))
                     / M_PI + 0.5);
        }
        v2[i] = v2[0];
    }
    for (i = 0; i < res; i++) {
        switch (texture) {
          case NATURAL:
          case SPHERICAL:
            vertex_tx_push(x_arr[i] * factor, y_arr[i] * factor, zradprim, 
                           u_arr[i], v2[i], 0.0);
            vertex_tx_push(factor * x_arr[i + 1], factor * y_arr[i + 1],
                           zradprim, 
                           u_arr[i + 1], v2[i + 1], 0.0);
            vertex_tx_push(0.0, 0.0, z_rad, u_arr[i + 1], 1.0, 0.0);
            vertex_tx_push(0.0, 0.0, z_rad, u_arr[i], 1.0, 0.0);
            break;

          case CYLINDRICAL:
            vertex_tx_push(x_arr[i] * factor, y_arr[i] * factor, zradprim, 
                           u_arr[i], zradprim / (2.0 * z_rad) + 0.5, 0.0);
            vertex_tx_push(factor * x_arr[i + 1], factor * y_arr[i + 1],
                           zradprim, 
                           u_arr[i + 1], zradprim / (2.0 * z_rad) + 0.5, 0.0);
            vertex_tx_push(0.0, 0.0, z_rad, u_arr[i + 1], 1.0, 0.0);
            vertex_tx_push(0.0, 0.0, z_rad, u_arr[i], 1.0, 0.0);
            break;

          case WORLD:
          default:
            vertex_tx_push(x_arr[i] * factor, y_arr[i] * factor, zradprim, 
                           x_arr[i] * factor, y_arr[i] * factor, zradprim);
            vertex_tx_push(factor * x_arr[i + 1],
                           factor * y_arr[i + 1],
                           zradprim, 
                           factor * x_arr[i + 1],
                           factor * y_arr[i + 1],
                           zradprim);
            vertex_tx_push(0.0, 0.0, z_rad, 0.0, 0.0, z_rad);
            break;
        }
        polygon_push();
    }

    /* Create the surface between the poles. */
    factor2 = factor;
    zradprim2 = zradprim;
    for (j = 1; j < res / 2 - 1; j++) {
        factor1 = factor2;
        factor2 = sin((j + 1) * M_PI / (res / 2));
        zradprim1 = zradprim2;
        zradprim2 = z_rad * cos((j + 1) * M_PI / (res / 2));
        if (texture == SPHERICAL || texture == NATURAL) {
            v1 = v2;
            for (i = 0; i < res; i++) {
                v2[i] = (atan(zradprim2
                              / sqrt(factor2 * factor2 
                                     * (x_arr[i] * x_arr[i] 
                                        + y_arr[i] * y_arr[i])))
                         / M_PI + 0.5);
            }
            v2[i] = v2[0];
        }

        for (i = 0; i < res; i++) {
            switch (texture) {
              case NATURAL:
              case SPHERICAL:
                vertex_tx_push(factor1 * x_arr[i], factor1 * y_arr[i], 
                               zradprim1, 
                               u_arr[i], v1[i], 0.0);
                vertex_tx_push(factor2 * x_arr[i], factor2 * y_arr[i], 
                               zradprim2, 
                               u_arr[i], v2[i], 0.0);
                vertex_tx_push(factor2 * x_arr[i + 1], factor2 * y_arr[i + 1],
                               zradprim2, 
                               u_arr[i + 1], v2[i + 1], 0.0);
                vertex_tx_push(factor1 * x_arr[i + 1], factor1 * y_arr[i + 1],
                               zradprim1, 
                               u_arr[i + 1], v1[i + 1], 0.0);
                break;

              case CYLINDRICAL:
                vertex_tx_push(factor1 * x_arr[i], factor1 * y_arr[i], 
                               zradprim1, 
                               u_arr[i], zradprim1 / (2.0 * z_rad) + 0.5, 0.0);
                vertex_tx_push(factor2 * x_arr[i], factor2 * y_arr[i], 
                               zradprim2, 
                               u_arr[i], zradprim2 / (2.0 * z_rad) + 0.5, 0.0);
                vertex_tx_push(factor2 * x_arr[i + 1], factor2 * y_arr[i + 1],
                               zradprim2, 
                               u_arr[i + 1], zradprim2 / (2.0 * z_rad) + 0.5, 
                               0.0);
                vertex_tx_push(factor1 * x_arr[i + 1], factor1 * y_arr[i + 1],
                               zradprim1, 
                               u_arr[i + 1], zradprim1 / (2.0 * z_rad) + 0.5, 
                               0.0);
                break;

              case WORLD:
              default:
                vertex_tx_push(factor1 * x_arr[i], factor1 * y_arr[i], 
                               zradprim1, 
                               factor1 * x_arr[i], factor1 * y_arr[i], 
                               zradprim1);
                vertex_tx_push(factor2 * x_arr[i], factor2 * y_arr[i], 
                               zradprim2, 
                               factor2 * x_arr[i], factor2 * y_arr[i], 
                               zradprim2);
                vertex_tx_push(factor2 * x_arr[i + 1], factor2 * y_arr[i + 1],
                               zradprim2, 
                               factor2 * x_arr[i + 1], factor2 * y_arr[i + 1],
                               zradprim2);
                vertex_tx_push(factor1 * x_arr[i + 1], factor1 * y_arr[i + 1],
                               zradprim1, 
                               factor1 * x_arr[i + 1], factor1 * y_arr[i + 1],
                               zradprim1);
            }
            polygon_push();
        }
    }

    /* Create the bottom pole */
    factor = sin(2.0 * M_PI / res);
    zradprim = -z_rad * cos(2.0 * M_PI / res);
    for (i = 0; i < res; i++) {
        switch (texture) {
          case NATURAL:
          case SPHERICAL:
            vertex_tx_push(factor * x_arr[i + 1], factor * y_arr[i + 1],
                           zradprim, 
                           u_arr[i + 1], v2[i + 1], 0.0);
            vertex_tx_push(x_arr[i] * factor, y_arr[i] * factor, zradprim, 
                           u_arr[i], v2[i], 0.0);
            vertex_tx_push(0.0, 0.0, -z_rad, 
                           u_arr[i], 0.0, 0.0);
            vertex_tx_push(0.0, 0.0, -z_rad, 
                           u_arr[i + 1], 0.0, 0.0);
            break;

          case CYLINDRICAL:
            vertex_tx_push(factor * x_arr[i + 1], factor * y_arr[i + 1],
                           zradprim, 
                           u_arr[i + 1], zradprim / (2.0 * z_rad) + 0.5, 0.0);
            vertex_tx_push(x_arr[i] * factor, y_arr[i] * factor, zradprim, 
                           u_arr[i], zradprim / (2.0 * z_rad) + 0.5, 0.0);
            vertex_tx_push(0.0, 0.0, -z_rad, 
                           u_arr[i], 0.0, 0.0);
            vertex_tx_push(0.0, 0.0, -z_rad, 
                           u_arr[i + 1], 0.0, 0.0);
            break;

          case WORLD:
          default:
            vertex_tx_push(x_arr[i + 1] * factor, y_arr[i + 1] * factor,
                           zradprim, 
                           x_arr[i + 1] * factor, y_arr[i + 1] * factor,
                           zradprim);
            vertex_tx_push(x_arr[i] * factor, y_arr[i] * factor, zradprim, 
                           x_arr[i] * factor, y_arr[i] * factor, zradprim);
            vertex_tx_push(0.0, 0.0, -z_rad, 0.0, 0.0, -z_rad);
            break;
        }
        polygon_push();
    }

    ellipsoid = object_create();

    old_user_refs = sipp_user_refcount (FALSE);
    object_add_surface(ellipsoid, surface_create(surface, shader));
    sipp_user_refcount (old_user_refs);

    return ellipsoid;
}


Object *
sipp_sphere(radius, res, surface, shader, texture)
    double  radius;
    int     res;
    void   *surface;
    Shader *shader;
    int     texture;
{
    return sipp_ellipsoid(radius, radius, radius, res, surface, shader,
                          texture);
}
