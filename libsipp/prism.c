/*
 * File: prism.c
 *
 * Create a prism from a polygonal cross-section in XY.
 * The points must be given in counter-clockwise order as viewed from
 * the front (+ve Z).
 *
 * Author:  David Jones
 *          djones@awesome.berkeley.edu
 *          11may91 djones
 *
 * Adapted for inclusion into the SIPP package: Inge Wallin
 * Extended with 2D texture support:            Jonas Yngvesson
 */

#include <math.h>
#include <stdio.h>

#include <primitives.h>
#include <sipp.h>
#include <xalloca.h>

Object *sipp_prism(int num_points, Vector *points, double length, void *surface,
                   Shader *shader, int texture) {
  Object *prism;
  double *u = NULL;
  double *v = NULL;
  int i;
  int j;

  switch (texture) {
  case NATURAL:
    u = (double *)alloca((num_points + 1) * sizeof(double));
    u[0] = 0.0;
    for (i = 1; i <= num_points; i++) {
      u[i] = u[i - 1] + sqrt(((points[i % num_points].x - points[i - 1].x) *
                              (points[i % num_points].x - points[i - 1].x)) +
                             ((points[i % num_points].y - points[i - 1].y) *
                              (points[i % num_points].y - points[i - 1].y)));
    }
    for (i = 0; i <= num_points; i++) {
      u[i] /= u[num_points];
    }
    break;

  case SPHERICAL:
    v = (double *)alloca((num_points + 1) * sizeof(double));
    for (i = 0; i < num_points; i++) {
      v[i] =
          (atan(length / 2.0 /
                sqrt(points[i].x * points[i].x + points[i].y * points[i].y)) /
               M_PI +
           0.5);
    }
    v[i] = v[0];
    /* Fall through */

  case CYLINDRICAL:
    u = (double *)alloca((num_points + 1) * sizeof(double));
    u[0] = atan2(points[0].y, points[0].x);
    for (i = 1; i < num_points; i++) {
      u[i] = atan2(points[i].y, points[i].x) - u[0];
      if (u[i] < 0.0) {
        u[i] += 2.0 * M_PI;
      }
      u[i] /= (2.0 * M_PI);
    }
    u[0] = 0.0;
    u[i] = 1.0;
    break;

  case WORLD:
  default:
    break;
  }

  prism = object_create();

  /* The top. */
  for (i = 0; i < num_points; ++i) {
    switch (texture) {
    case NATURAL:
    case CYLINDRICAL:
    case SPHERICAL:
      vertex_tx_push(points[i].x, points[i].y, length / 2.0, 0.0, 1.0, 0.0);
      break;

    case WORLD:
    default:
      vertex_tx_push(points[i].x, points[i].y, length / 2.0, points[i].x,
                     points[i].y, length / 2.0);
      break;
    }
  }
  polygon_push();
  object_add_surface(prism, surface_create(surface, shader));

  /* The bottom */
  for (i = num_points - 1; i >= 0; --i) {
    switch (texture) {
    case NATURAL:
    case CYLINDRICAL:
    case SPHERICAL:
      vertex_tx_push(points[i].x, points[i].y, -length / 2.0, 0.0, 0.0, 0.0);
      break;

    case WORLD:
    default:
      vertex_tx_push(points[i].x, points[i].y, -length / 2.0, points[i].x,
                     points[i].y, -length / 2.0);
      break;
    }
  }
  polygon_push();
  object_add_surface(prism, surface_create(surface, shader));

  /* The sides */
  for (i = 0; i < num_points; ++i) {
    j = i + 1;
    if (j == num_points)
      j = 0;
    switch (texture) {
    case NATURAL:
    case CYLINDRICAL:
      vertex_tx_push(points[i].x, points[i].y, length / 2.0, u[i], 1.0, 0.0);
      vertex_tx_push(points[i].x, points[i].y, -length / 2.0, u[i], 0.0, 0.0);
      vertex_tx_push(points[j].x, points[j].y, -length / 2.0, u[i + 1], 0.0,
                     0.0);
      vertex_tx_push(points[j].x, points[j].y, length / 2.0, u[i + 1], 1.0,
                     0.0);
      break;

    case SPHERICAL:
      vertex_tx_push(points[i].x, points[i].y, length / 2.0, u[i], v[i], 0.0);
      vertex_tx_push(points[i].x, points[i].y, -length / 2.0, u[i], 1.0 - v[i],
                     0.0);
      vertex_tx_push(points[j].x, points[j].y, -length / 2.0, u[i + 1],
                     1.0 - v[i + 1], 0.0);
      vertex_tx_push(points[j].x, points[j].y, length / 2.0, u[i + 1], v[i + 1],
                     0.0);
      break;

    case WORLD:
    default:
      vertex_tx_push(points[i].x, points[i].y, length / 2.0, points[i].x,
                     points[i].y, length / 2.0);
      vertex_tx_push(points[i].x, points[i].y, -length / 2.0, points[i].x,
                     points[i].y, -length / 2.0);
      vertex_tx_push(points[j].x, points[j].y, -length / 2.0, points[j].x,
                     points[j].y, -length / 2.0);
      vertex_tx_push(points[j].x, points[j].y, length / 2.0, points[j].x,
                     points[j].y, length / 2.0);
      break;
    }
    polygon_push();
    object_add_surface(prism, surface_create(surface, shader));
  }

  return prism;
}

/*
 * A square block. Generated as a prism.
 */
Object *sipp_block(double xsize, double ysize, double zsize, void *surface,
                   Shader *shader, int texture) {
  Vector coor[4];

  xsize /= 2.0;
  ysize /= 2.0;

  coor[0].x = xsize;
  coor[0].y = -ysize;
  coor[1].x = xsize;
  coor[1].y = ysize;
  coor[2].x = -xsize;
  coor[2].y = ysize;
  coor[3].x = -xsize;
  coor[3].y = -ysize;

  return sipp_prism(4, coor, zsize, surface, shader, texture);
}

/*
 * A cube.
 */
Object *sipp_cube(double size, void *surface, Shader *shader, int texture) {
  return sipp_block(size, size, size, surface, shader, texture);
}
