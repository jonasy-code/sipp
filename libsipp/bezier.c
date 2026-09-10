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
 ** bezier.c - Reading bezier descriptions and creating objects.
 **/

#include <math.h>
#include <stdio.h>

#include <sipp.h>

#include <bezier.h>
#include <primitives.h>
#include <smalloc.h>

Tokenval tokenval;

/*
 * Prototypes of internal functions.
 */
static void vertex_read(Bez_Object *obj);

static void curve_read(Bez_Object *obj);

static void patch_read(Bez_Object *obj);

static Bez_Object *bezier_read(FILE *file);

static Surface *bezier_patches(Bez_Object *obj, int res, void *surface,
                               Shader *shader, int texture);

static double C(int i);

static double bblend(int i, double u);

static void bez_curve_eval(Vector *vertex, Bez_Curve *curve, double u,
                           double *x, double *y, double *z);

static void bez_patch_eval(Vector *vertex, Bez_Patch *patch, double u, double v,
                           double *x, double *y, double *z);

static Surface *bezier_rot_curves(Bez_Object *obj, int res, void *surface,
                                  Shader *shader, int texture);

/*================================================================*/
/*                                                                */
/*      Functions for reading bezier descriptions from file       */
/*                                                                */
/*================================================================*/

/*
 * Read a vertex list, where each vertex is an xyz triple, and
 * install it in the bezier structure.
 */
static void vertex_read(Bez_Object *obj) {
  int token;
  int i, j;

  token = bezier_lex();
  if (token != NVERTICES) {
    fprintf(stderr, "Corrupt vertex description.\n");
    goto errout;
  }

  token = bezier_lex();
  if (token != INTEGER) {
    fprintf(stderr, "Corrupt vertex description.\n");
    goto errout;
  }
  obj->nvertex = tokenval.intval;
  obj->vertex = (Vector *)smalloc(obj->nvertex * sizeof(Vector));

  token = bezier_lex();
  if (token != VERTEX_LIST) {
    fprintf(stderr, "Corrupt vertex description.\n");
    goto errout;
  }

  for (i = 0; i < obj->nvertex; i++) {
    for (j = 0; j < 3; j++) {
      token = bezier_lex();
      if (token != FLOAT && token != INTEGER) {
        fprintf(stderr, "Corrupt vertex description.\n");
        goto errout;
      }
      switch (j) {
      case 0:
        if (token == FLOAT) {
          obj->vertex[i].x = tokenval.floatval;
        } else {
          obj->vertex[i].x = (double)tokenval.intval;
        }
        break;

      case 1:
        if (token == FLOAT) {
          obj->vertex[i].y = tokenval.floatval;
        } else {
          obj->vertex[i].y = (double)tokenval.intval;
        }
        break;

      case 2:
        if (token == FLOAT) {
          obj->vertex[i].z = tokenval.floatval;
        } else {
          obj->vertex[i].z = (double)tokenval.intval;
        }
      }
    }
  }

  return;

errout:
  if (obj->vertex != NULL) {
    sfree(obj->vertex);
    obj->vertex = NULL;
  }
}

/*
 * Read a list of bezier curves, where each curve consists of
 * four control points (index into the vertex list), and install
 * it in the bezier structure.
 */
static void curve_read(Bez_Object *obj) {
  int token;
  int i, j;

  token = bezier_lex();
  if (token != NCURVES) {
    fprintf(stderr, "Corrupt curve description.\n");
    goto errout;
  }

  token = bezier_lex();
  if (token != INTEGER) {
    fprintf(stderr, "Corrupt curve description.\n");
    goto errout;
  }
  obj->n.ncurves = tokenval.intval;
  obj->cp.ccp = (Bez_Curve *)smalloc(obj->n.ncurves * sizeof(Bez_Curve));

  token = bezier_lex();
  if (token != CURVE_LIST) {
    fprintf(stderr, "Corrupt curve description.\n");
    goto errout;
  }

  for (i = 0; i < obj->n.ncurves; i++) {
    for (j = 0; j < 4; j++) {
      token = bezier_lex();
      if (token != INTEGER) {
        fprintf(stderr, "Corrupt curve description.\n");
        goto errout;
      }
      obj->cp.ccp[i].cp[3 - j] = tokenval.intval - 1;
    }
  }

  return;

errout:
  if (obj->cp.ccp != NULL) {
    sfree(obj->cp.ccp);
    obj->cp.ccp = NULL;
  }
}

/*
 * Read a list of bezier patches, where each patch consists of
 * sixteen control points (index into the vertex list), and install
 * it in the bezier structure.
 */
static void patch_read(Bez_Object *obj) {
  int token;
  int i, j, k;

  token = bezier_lex();
  if (token != NPATCHES) {
    fprintf(stderr, "Corrupt patch description.\n");
    goto errout;
  }

  token = bezier_lex();
  if (token != INTEGER) {
    fprintf(stderr, "Corrupt patch description.\n");
    goto errout;
  }
  obj->n.npatches = tokenval.intval;
  obj->cp.pcp = (Bez_Patch *)smalloc(obj->n.npatches * sizeof(Bez_Patch));

  token = bezier_lex();
  if (token != PATCH_LIST) {
    fprintf(stderr, "Corrupt patch description.\n");
    goto errout;
  }

  for (i = 0; i < obj->n.npatches; i++) {
    for (j = 0; j < 4; j++) {
      for (k = 0; k < 4; k++) {
        token = bezier_lex();
        if (token != INTEGER) {
          fprintf(stderr, "Corrupt patch description.\n");
          goto errout;
        }
        obj->cp.pcp[i].cp[j][k] = tokenval.intval - 1;
      }
    }
  }

  return;

errout:
  if (obj->cp.pcp != NULL) {
    sfree(obj->cp.pcp);
    obj->cp.pcp = NULL;
  }
}

/*
 * Read a bezier object from a file, i.e. determine if it is a
 * curve or patch description,  read vertex and curve or patch
 * description. Build a bezier object from the data.
 */
static Bez_Object *bezier_read(FILE *file) {
  int token;
  Bez_Object *obj;

  bezier_lex_open(file);

  obj = (Bez_Object *)scalloc(1, sizeof(Bez_Object));
  if ((token = bezier_lex()) == PATCHES) {
    obj->type = PATCHES;
    vertex_read(obj);
    if (obj->vertex == NULL) {
      goto errout;
    }
    patch_read(obj);
    if (obj->cp.pcp == NULL) {
      goto errout;
    }
  } else if (token == CURVES) {
    obj->type = CURVES;
    vertex_read(obj);
    if (obj->vertex == NULL) {
      goto errout;
    }
    curve_read(obj);
    if (obj->cp.ccp == NULL) {
      goto errout;
    }
  } else {
    fprintf(stderr, "Corrupt bezier description file.\n");
    bezier_lex_close();
    return NULL;
  }

  bezier_lex_close();
  return obj;

errout:
  bezier_lex_close();
  if (obj != NULL) {
    if (obj->vertex != NULL) {
      sfree(obj->vertex);
    }
    if (obj->cp.pcp != NULL) {
      sfree(obj->cp.pcp);
    }
    sfree(obj);
  }
  return NULL;
}

/*================================================================*/
/*                                                                */
/*           Functions for evaluating bezier functions            */
/*                                                                */
/*================================================================*/

static double C(int i) {
  int j, a;

  a = 1;
  for (j = i + 1; j < 4; j++) {
    a = a * j;
  }
  for (j = 1; j < 4 - i; j++) {
    a = a / j;
  }

  return (double)a;
}

static double bblend(int i, double u) {
  int j;
  double v;

  v = C(i);
  for (j = 1; j <= i; j++) {
    v *= u;
  }
  for (j = 1; j < 4 - i; j++) {
    v *= 1.0 - u;
  }

  if (fabs(v) < 1.0E-15) {
    return 0.0;
  } else {
    return v;
  }
}

/*
 * Determine x, y and z coordinates of a point on the bezier
 * curve described by CURVE and VERTEX. U is a parameter between
 * 0 and 1 that determines how far "into" the curve we are.
 */
static void bez_curve_eval(Vector *vertex, Bez_Curve *curve, double u,
                           double *x, double *y, double *z) {
  int i;
  double b;

  *x = 0;
  *y = 0;
  *z = 0;

  for (i = 0; i < 4; i++) {
    b = bblend(i, u);
    *x += vertex[curve->cp[i]].x * b;
    *y += vertex[curve->cp[i]].y * b;
    *z += vertex[curve->cp[i]].z * b;
  }
}

/*
 * Determine x, y and z coordinates of a point on the bezier
 * patch described by PATCH and VERTEX. U and V are parameters
 * between 0 and 1 that determines where on the patch we are.
 */
static void bez_patch_eval(Vector *vertex, Bez_Patch *patch, double v, double u,
                           double *x, double *y, double *z) {
  int i, j;
  double b;

  *x = 0;
  *y = 0;
  *z = 0;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      b = bblend(i, v);
      b *= bblend(j, u);
      *x += vertex[patch->cp[i][j]].x * b;
      *y += vertex[patch->cp[i][j]].y * b;
      *z += vertex[patch->cp[i][j]].z * b;
    }
  }
}

/*================================================================*/
/*                                                                */
/*             Functions for creating bezier objects              */
/*             (these functions are SIPP specific)                */
/*                                                                */
/*================================================================*/

/*
 * Approximate the bezier patches described in OBJ with polygons
 * and create a SIPP surface out of them. The patches will be
 * tesselated into RESxRES polygons (rectangles).
 */
static Surface *bezier_patches(Bez_Object *obj, int res, void *surface,
                               Shader *shader, int texture) {
  double x, y, z;
  double u, v;
  double step;
  int i, j, k;

  step = 1.0 / (double)res;

  for (i = 0; i < obj->n.npatches; i++) {
    for (v = 0.0, j = 0; j < res; j++, v = j * step) {
      for (u = 0.0, k = 0; k < res; k++, u = k * step) {
        switch (texture) {
        case NATURAL:
        case CYLINDRICAL:
        case SPHERICAL:
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v, u, &x, &y, &z);
          vertex_tx_push(x, y, z, u, v, 0.0);
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v, u + step, &x, &y, &z);
          vertex_tx_push(x, y, z, u + step, v, 0.0);
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v + step, u + step, &x,
                         &y, &z);
          vertex_tx_push(x, y, z, u + step, v + step, 0.0);
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v + step, u, &x, &y, &z);
          vertex_tx_push(x, y, z, u, v + step, 0.0);
          break;

        case WORLD:
        default:
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v, u, &x, &y, &z);
          vertex_tx_push(x, y, z, x, y, z);
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v, u + step, &x, &y, &z);
          vertex_tx_push(x, y, z, x, y, z);
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v + step, u + step, &x,
                         &y, &z);
          vertex_tx_push(x, y, z, x, y, z);
          bez_patch_eval(obj->vertex, &obj->cp.pcp[i], v + step, u, &x, &y, &z);
          vertex_tx_push(x, y, z, x, y, z);
          break;
        }
        polygon_push();
      }
    }
  }
  return surface_create(surface, shader);
}

/*
 * Take the bezier curves described in OBJ and create a
 * surface by rotating them about th y-axis. The object
 * will be tesselated as RESx(RES*4) polygons per curve.
 * (The reason for using 4 times the resolution is rather
 * "handwavy". I think 90 degrees is about as much as a
 * patch should cover of a rotational body.)
 */
static Surface *bezier_rot_curves(Bez_Object *obj, int res, void *surface,
                                  Shader *shader, int texture) {
  double x[4], y[4], z[4];
  double u;
  double v;
  double step;
  double vstep;
  double ca, sa;
  int i, j, k;

  step = 1.0 / (double)res;
  vstep = step / 4.0;
  ca = cos(2.0 * M_PI / (4.0 * (double)res));
  sa = sin(2.0 * M_PI / (4.0 * (double)res));

  for (i = 0; i < obj->n.ncurves; i++) {
    for (u = 0.0, j = 0; j < res; j++, u += step) {
      bez_curve_eval(obj->vertex, &obj->cp.ccp[i], u, &x[0], &y[0], &z[0]);
      bez_curve_eval(obj->vertex, &obj->cp.ccp[i], u + step, &x[1], &y[1],
                     &z[1]);
      z[3] = z[0];
      z[2] = z[1];
      for (k = 0; k < 4 * res; k++) {
        x[3] = ca * x[0] - sa * y[0];
        y[3] = ca * y[0] + sa * x[0];
        x[2] = ca * x[1] - sa * y[1];
        y[2] = ca * y[1] + sa * x[1];
        switch (texture) {
        case NATURAL:
        case CYLINDRICAL:
        case SPHERICAL:
          v = k * vstep;
          vertex_tx_push(x[0], y[0], z[0], v, u, 0.0);
          vertex_tx_push(x[3], y[3], z[3], v + vstep, u, 0.0);
          vertex_tx_push(x[2], y[2], z[2], v + vstep, u + step, 0.0);
          vertex_tx_push(x[1], y[1], z[1], v, u + step, 0.0);
          break;

        case WORLD:
        default:
          vertex_tx_push(x[0], y[0], z[0], x[0], y[0], z[0]);
          vertex_tx_push(x[3], y[3], z[3], x[3], y[3], z[3]);
          vertex_tx_push(x[2], y[2], z[2], x[2], y[2], z[2]);
          vertex_tx_push(x[1], y[1], z[1], x[1], y[1], z[1]);
          break;
        }
        polygon_push();
        x[0] = x[3];
        y[0] = y[3];
        x[1] = x[2];
        y[1] = y[2];
      }
    }
  }

  return surface_create(surface, shader);
}

/*
 * Read a bezier description from FILE and build
 * a bezier surface. Tesselate this object into
 * polygons and return a pointer to a SIPP object.
 */
Object *sipp_bezier_file(FILE *file, int res, void *surface, Shader *shader,
                         int texture) {
  Object *obj;
  Bez_Object *bez_obj;
  bool old_user_refs;

  bez_obj = bezier_read(file);
  if (bez_obj == NULL) {
    return NULL;
  }

  obj = object_create();
  old_user_refs = sipp_user_refcount(FALSE);
  if (bez_obj->type == PATCHES) {
    object_add_surface(obj,
                       bezier_patches(bez_obj, res, surface, shader, texture));
    sfree(bez_obj->vertex);
    sfree(bez_obj->cp.pcp);
    sfree(bez_obj);
  } else {
    object_add_surface(
        obj, bezier_rot_curves(bez_obj, res, surface, shader, texture));
    sfree(bez_obj->vertex);
    sfree(bez_obj->cp.ccp);
    sfree(bez_obj);
  }
  sipp_user_refcount(old_user_refs);

  return obj;
}

/*
 * Define an object directly from lists of vertices and control points for
 * one or more bezier patches.
 */
Object *sipp_bezier_patches(int nvert, Vector *vertex, int npatch,
                            int *cp_index, int res, void *surface,
                            Shader *shader, int texture) {
  Object *obj;
  Bez_Object bez_obj;
  int index;
  int i, j, k;
  bool old_user_refs;

  bez_obj.type = PATCHES;

  bez_obj.nvertex = nvert;
  bez_obj.vertex = vertex;

  bez_obj.n.npatches = npatch;
  bez_obj.cp.pcp = (Bez_Patch *)scalloc(npatch, sizeof(Bez_Patch));

  for (i = 0, index = 0; i < npatch; i++) {
    for (j = 0; j < 4; j++) {
      for (k = 0; k < 4; k++, index++) {
        bez_obj.cp.pcp[i].cp[j][k] = cp_index[index];
      }
    }
  }

  obj = object_create();

  old_user_refs = sipp_user_refcount(FALSE);
  object_add_surface(obj,
                     bezier_patches(&bez_obj, res, surface, shader, texture));
  sipp_user_refcount(old_user_refs);

  sfree(bez_obj.cp.pcp);

  return obj;
}

/*
 * Define an object directly from lists of vertices and control points for one
 * ore more bezier curves. The curves will be rotated about the world y-axis to
 * create rotational bodies.
 */
Object *sipp_bezier_rotcurve(int nvert, Vector *vertex, int ncurve,
                             int *cp_index, int res, void *surface,
                             Shader *shader, int texture) {
  Object *obj;
  Bez_Object bez_obj;
  int index;
  int i, j;
  bool old_user_refs;

  bez_obj.type = CURVES;

  bez_obj.nvertex = nvert;
  bez_obj.vertex = vertex;

  bez_obj.n.npatches = ncurve;
  bez_obj.cp.ccp = (Bez_Curve *)scalloc(ncurve, sizeof(Bez_Curve));

  for (i = 0, index = 0; i < ncurve; i++) {
    for (j = 0; j < 4; j++, index++) {
      bez_obj.cp.ccp[i].cp[j] = cp_index[index];
    }
  }

  obj = object_create();

  old_user_refs = sipp_user_refcount(FALSE);
  object_add_surface(
      obj, bezier_rot_curves(&bez_obj, res, surface, shader, texture));
  sipp_user_refcount(old_user_refs);

  sfree(bez_obj.cp.ccp);

  return obj;
}
