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
 ** torus.c - Creating a torus as a sipp object.
 **/

#include <xalloca.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>


#define SWAPARR(a, b) {Vector *tmp; tmp = a; a = b; b = tmp;}


static Vector *arr1;
static Vector *arr2;
static Vector *tx1;
static Vector *tx2;

/*
 * Prototypes of internal functions.
 */
static void
arr_rot _ANSI_ARGS_((int    len,
                     double angle,
                     int    texture));

static void
push_band _ANSI_ARGS_((int    len));


static void
arr_rot(len, angle, texture)
    int    len;
    double angle;
    int    texture;
{
    int    i;
    double sa, ca;

    sa = sin(angle);
    ca = cos(angle);
    for (i = 0; i < len; i++) {
        arr2[i].x = arr1[i].x * ca - arr1[i].y * sa;
        arr2[i].y = arr1[i].x * sa + arr1[i].y * ca;
        arr2[i].z = arr1[i].z;
        switch (texture) {
          case NATURAL:
          case CYLINDRICAL:
          case SPHERICAL:
            tx2[i].x = tx1[i].x + angle / (2.0 * M_PI);
            tx2[i].y = tx1[i].y;
            tx2[i].z = tx1[i].z;
            break;

          case WORLD:
          default:
            tx2[i] = arr2[i];
            break;
        }
    }
    switch (texture) {
      case NATURAL:
      case CYLINDRICAL:
      case SPHERICAL:
        tx2[i].x = tx1[i].x + angle / (2.0 * M_PI);
        tx2[i].y = tx1[i].y;
        tx2[i].z = tx1[i].z;
        break;

      case WORLD:
      default:
        tx2[i] = tx2[0];
        break;
    }
}


static void
push_band(len)
    int    len;
{
    int i, j;

    for (i = 0; i < len; i++) {
        j = (i + 1) % len;
        vertex_tx_push(arr1[i].x, arr1[i].y, arr1[i].z, 
                       tx1[i].x, tx1[i].y, tx1[i].z);
        vertex_tx_push(arr2[i].x, arr2[i].y, arr2[i].z, 
                       tx2[i].x, tx2[i].y, tx2[i].z);
        vertex_tx_push(arr2[j].x, arr2[j].y, arr2[j].z, 
                       tx2[i + 1].x, tx2[i + 1].y, tx2[i + 1].z);
        vertex_tx_push(arr1[j].x, arr1[j].y, arr1[j].z, 
                       tx1[i + 1].x, tx1[i + 1].y, tx1[i + 1].z);
        polygon_push();
    }
}



Object *
sipp_torus(bigradius, smallradius, res1, res2, surface, shader, texture)
    double  bigradius;      /* Radius of the ring */
    double  smallradius;    /* Radius of the "tube" */
    int     res1;           /* Number of polygons around the ring */
    int     res2;           /* Number of polygons around the tube */
    void   *surface;
    Shader *shader;
    int     texture;
{
    Object *torus;
    double  angle;
    int     i;
    bool    old_user_refs;

    /* Create two arrays to hold vertices around the tube */
    arr1 = (Vector *)alloca(res2 * sizeof(Vector));
    arr2 = (Vector *)alloca(res2 * sizeof(Vector));

    /* Create two arrays to hold texture coordinates  around the tube */
    tx1 = (Vector *)alloca((res2 + 1) * sizeof(Vector));
    tx2 = (Vector *)alloca((res2 + 1) * sizeof(Vector));

    for (i = 0; i < res2; i++) {
        angle = i * 2.0 * M_PI / res2 - M_PI / 4.0;
        arr1[i].x = bigradius + smallradius * cos(angle);
        arr1[i].y = 0.0;
        arr1[i].z = smallradius * sin(angle);
        switch (texture) {
          case NATURAL:
            tx1[i].x = 0.0;
            tx1[i].y = (double)i / (double)res2;
            tx1[i].z = 0.0;
            break;

          case CYLINDRICAL:
            tx1[i].x = 0.0;
            tx1[i].y = (arr1[i].z + smallradius) / (2.0 * smallradius);
            tx1[i].z = 0.0;
            break;

          case SPHERICAL:
            tx1[i].x = 0.0;
            tx1[i].y = atan(arr1[i].z / arr1[i].x) / M_PI + 0.5;
            tx1[i].z = 0.0;
            break;

          case WORLD:
          default:
            tx1[i] = arr1[i];
            break;
        }
    }

    switch (texture) {
      case NATURAL:
        tx1[i].x = 0.0;
        tx1[i].y = (double)i / (double)res2;
        tx1[i].z = 0.0;
        break;
        
      case CYLINDRICAL:
        tx1[i].x = 0.0;
        tx1[i].y = (arr1[0].z + smallradius) / (2.0 * smallradius);
        tx1[i].z = 0.0;
        break;
        
      case SPHERICAL:
        tx1[i].x = 0.0;
        tx1[i].y = atan(arr1[0].z / arr1[0].x) / M_PI + 0.5;
        tx1[i].z = 0.0;
        break;
        
      case WORLD:
      default:
        tx1[i] = tx1[0];
        break;
    }
    
    /* Sweep out the torus by rotating the two perimeters */
    /* defined in arr1 and arr2. */
    for (i = 0; i < res1; i++) {
        arr_rot(res2, 2.0 * M_PI / (double)res1, texture);
        push_band(res2);
        SWAPARR(arr1, arr2)
        SWAPARR(tx1, tx2)
    }

    torus = object_create();

    old_user_refs = sipp_user_refcount (FALSE);
    object_add_surface(torus, surface_create(surface, shader));
    sipp_user_refcount (old_user_refs);

    return torus;
}

