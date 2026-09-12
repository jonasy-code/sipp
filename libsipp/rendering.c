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
 ** rendering.c - Functions that handles rendering of the scene.
 **/

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <sipp.h>

#include <arena.h>
#include <geometric.h>
#include <lightsource.h>
#include <objects.h>
#include <pixelbuf.h>
#include <rendering.h>
#include <sipp_bitmap.h>
#include <sipp_png.h>
#include <smalloc.h>
#include <viewpoint.h>
#include <xalloca.h>

#ifndef NOMEMCPY
#include <memory.h>
#endif

#include "patchlevel.h"
char *SIPP_VERSION = VERSION;

#ifndef MAXFLOAT
#define MAXFLOAT ((float)3.40282346638528860e+38)
#endif

/*
 * Static global variables.
 */
static bool show_backfaces;    /* Don't do backface culling */
static bool reverse_scan;      /* Render scan lines in reverse (down to up) */
static bool auto_shadows;      /* Calculate shadows */
static bool shading_per_pixel; /* Shade each polygon once per pixel */
static Edge **y_bucket;        /* Y-bucket for edge lists. */
static FILE *image_file;       /* File to store image in      */
                               /* when rendering into a file. */
static Image_format image_format; /* Its format: IMAGE_PPM or IMAGE_PNG */
static bool image_alpha;          /* The image has an alpha channel */
static unsigned char *png_rows;   /* The rows of a PNG image are    */
static int png_nrows;             /* collected here until all of    */
static int png_lines;             /* them are known                 */
static int png_rowbytes;
static Pixel_func *pixel_set; /* User function receiving pixels   */
                              /* when rendering with a function.  */
static Line_func *line_set;   /* Likewise for lines in LINE mode. */
static void *im_data;         /* Data to pixel_set()/line_set()   */

int depthmap_size; /* Size of the depthmaps */

/*
 * Arenas for the two kinds of short-lived rendering objects.
 *
 * edge_arena holds all Edges of one rendering pass (the main image or
 * one depthmap).  Edges are created during the object tree traversal
 * and consumed by the scanline sweep; the arena is reset before each
 * traversal.
 *
 * vcoord_arena holds the View_coords of the polygon currently being
 * transformed and clipped, and is reset for every polygon.
 */
enum { EDGE_ARENA_BLOCK = 256 * 1024, VCOORD_ARENA_BLOCK = 16 * 1024 };

static Arena edge_arena;
static Arena vcoord_arena;
static int edge_count;         /* Edges created in this pass */
static int render_threads = 1; /* Threads to render with */

/*
 * Flag that can be set to TRUE to terminate the rendering.
 */
static volatile bool abort_render;

/*
 * Function to call during rendering process to check for window events, etc,
 */
static Update_func *update_func = NULL;
static void *update_client_data = NULL;
static int update_period = 0;
static int update_count = 0;

/*
 * This macro is used to handle the calls to the update call back.  The
 * count is updated and the update proc is called when the period is reached.
 * The approimate unit of the count is the amount of time it takes to render
 * one pixel.
 */
#define UPDATE_CALLBACK                                                        \
  if ((update_func != NULL) && (++update_count > update_period)) {             \
    update_count = 0;                                                          \
    (*update_func)(update_client_data);                                        \
  }

/*
 * Stack of transformation matrices used
 * when traversing an object hierarchy.
 */
static struct tm_stack_t {
  Transf_mat mat;
  struct tm_stack_t *next;
} *tm_stack;

static Transf_mat curr_mat; /* Current transformation matrix */

/*
 * The clipping planes of the view volume, see polygon_clip().
 */
typedef enum { XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX } Clip_plane;

/*
 * Prototypes of internal functions.
 */
static void calc_normals(Polygon *pstart, Vector eyepoint);

static void create_edges(View_coord *view_vert, int polygon, Surface *surface,
                         Render_mode render_mode);

static View_coord *interpolate(View_coord *v1, View_coord *v2, double ratio);

static void reset_normals(Vertex *vref);

static View_coord *polygon_clip(View_coord *vlist, Clip_plane plane,
                                bool first_vert);

static void transf_vertices(Vertex *vertex[], int nvertices, Surface *surface,
                            Transf_mat *view_mat, Transf_mat *tr_mat,
                            double xsiz, double ysiz, Render_mode render_mode);

#ifdef FFD
static void do_ffd(Vertex *vertex, Surface *surface);

static void ffd_vertices(Surface *surface);
#endif

static void scan_and_render(int xres, int yres, Storage_mode storage_mode,
                            Render_mode render_mode, int oversampl,
                            Field field);

static void matrix_push(void);

static void matrix_pop(void);

static void traverse_object_tree(Object *object, Transf_mat *view_mat, int xres,
                                 int yres, Render_mode render_mode);

static void scan_depthmap(float *d_map);

static void render_main(int xres, int yres, Storage_mode storage_mode,
                        Render_mode render_mode, int oversampling, Field field);

/*
 * Line_func adapter for the internal bitmap used when a LINE image is
 * written to a file.
 */
static void bitmap_line(void *bm, int x1, int y1, int x2, int y2) {
  sipp_bitmap_line((Sipp_bitmap *)bm, x1, y1, x2, y2);
}

/*
 * Calculate the normal vector for all polygons in the polygon list PSTART.
 *
 * Check if the polygon is backfacing with respect to the current
 * viewpoint.
 *
 * The normalized normal is added to a normal kept at each vertex
 * in the polygon. This will produce, at each vertex, an average of the
 * normals of the adjectent plygons.
 */
static void calc_normals(Polygon *pstart, Vector eyepoint) {
  Polygon *polyref;
  Vertex **vlist;
  Vector normal;
  int i, j;
  double plane_const;

  for (polyref = pstart; polyref != NULL; polyref = polyref->next) {

    vlist = polyref->vertex;
    MakeVector(normal, 0.0, 0.0, 0.0);

    for (i = 0; i < polyref->nvertices; i++) {
      j = (i + 1) % polyref->nvertices;
#ifdef FFD
      if (vlist[i]->ffd_vertex == NULL) {
#endif
        normal.x += ((vlist[i]->pos.y - vlist[j]->pos.y) *
                     (vlist[i]->pos.z + vlist[j]->pos.z));
        normal.y += ((vlist[i]->pos.z - vlist[j]->pos.z) *
                     (vlist[i]->pos.x + vlist[j]->pos.x));
        normal.z += ((vlist[i]->pos.x - vlist[j]->pos.x) *
                     (vlist[i]->pos.y + vlist[j]->pos.y));
#ifdef FFD
      } else {
        normal.x +=
            ((vlist[i]->ffd_vertex->pos.y - vlist[j]->ffd_vertex->pos.y) *
             (vlist[i]->ffd_vertex->pos.z + vlist[j]->ffd_vertex->pos.z));
        normal.y +=
            ((vlist[i]->ffd_vertex->pos.z - vlist[j]->ffd_vertex->pos.z) *
             (vlist[i]->ffd_vertex->pos.x + vlist[j]->ffd_vertex->pos.x));
        normal.z +=
            ((vlist[i]->ffd_vertex->pos.x - vlist[j]->ffd_vertex->pos.x) *
             (vlist[i]->ffd_vertex->pos.y + vlist[j]->ffd_vertex->pos.y));
      }
#endif
    }
    vecnorm(&normal);

    /*
     * Take care of backfacing polygons.
     */
#ifdef FFD
    if (vlist[0]->ffd_vertex == NULL) {
#endif
      plane_const = VecDot(normal, vlist[0]->pos);
#ifdef FFD
    } else {
      plane_const = VecDot(normal, vlist[0]->ffd_vertex->pos);
    }
#endif
    if (VecDot(eyepoint, normal) - plane_const <= 0.0) {
      if (show_backfaces) {
        polyref->backface = FALSE;
        VecNegate(normal);
      } else {
        polyref->backface = TRUE;
      }
    } else {
      polyref->backface = FALSE;
    }

    /*
     * Add the calculated normal to all vertices
     * in the poygon. This will result in an avaraged normal
     * at each vertex after all polygons have been processed.
     */
    for (i = 0; i < polyref->nvertices; i++) {
      if (!vlist[i]->fixed_normal) {
        VecAdd(vlist[i]->normal, vlist[i]->normal, normal);
      }
    }
  }
}

/*
 * Gradient over the screen of an attribute that is affine in screen
 * space, from its values A0, A1, A2 at three vertices; (X1, Y1) and
 * (X2, Y2) are the positions of the last two relative to the first, and
 * INV_DET is 1 / (X1 * Y2 - X2 * Y1).
 */
static void plane_gradient(double a0, double a1, double a2, double x1,
                           double y1, double x2, double y2, double inv_det,
                           double *gx, double *gy) {
  double d1 = a1 - a0, d2 = a2 - a0;

  *gx = (d1 * y2 - d2 * y1) * inv_det;
  *gy = (d2 * x1 - d1 * x2) * inv_det;
}

/*
 * Compute the gradients (see Poly_grad) of the polygon VIEW_VERT, a
 * circular list of vertices in screen coordinates whose texture and
 * world are already divided by w.  Three vertices spanning as large a
 * triangle as possible are used, for precision; a polygon that is
 * degenerate on the screen gets zero gradients.
 */
static void poly_gradients(View_coord *view_vert, Poly_grad *g) {
  View_coord *v, *v0, *v1, *v2;
  double dx, dy, d, best, det, inv_det;
  double x1, y1, x2, y2;

  v0 = view_vert;
  v1 = v2 = NULL;
  best = -1.0;
  for (v = v0->next; v != v0; v = v->next) {
    dx = v->view.x - v0->view.x;
    dy = v->view.y - v0->view.y;
    d = dx * dx + dy * dy;
    if (d > best) {
      best = d;
      v1 = v;
    }
  }
  best = 0.0;
  if (v1 != NULL) {
    x1 = v1->view.x - v0->view.x;
    y1 = v1->view.y - v0->view.y;
    for (v = v0->next; v != v0; v = v->next) {
      det = x1 * (v->view.y - v0->view.y) - (v->view.x - v0->view.x) * y1;
      if (fabs(det) > best) {
        best = fabs(det);
        v2 = v;
      }
    }
  }
  if (v2 == NULL || best < 1e-9) {
    memset(g, 0, sizeof(*g));
    return;
  }
  x1 = v1->view.x - v0->view.x;
  y1 = v1->view.y - v0->view.y;
  x2 = v2->view.x - v0->view.x;
  y2 = v2->view.y - v0->view.y;
  inv_det = 1.0 / (x1 * y2 - x2 * y1);

  plane_gradient(v0->texture.x, v1->texture.x, v2->texture.x, x1, y1, x2, y2,
                 inv_det, &g->dtex_dx.x, &g->dtex_dy.x);
  plane_gradient(v0->texture.y, v1->texture.y, v2->texture.y, x1, y1, x2, y2,
                 inv_det, &g->dtex_dx.y, &g->dtex_dy.y);
  plane_gradient(v0->texture.z, v1->texture.z, v2->texture.z, x1, y1, x2, y2,
                 inv_det, &g->dtex_dx.z, &g->dtex_dy.z);
  plane_gradient(v0->world.x, v1->world.x, v2->world.x, x1, y1, x2, y2, inv_det,
                 &g->dworld_dx.x, &g->dworld_dy.x);
  plane_gradient(v0->world.y, v1->world.y, v2->world.y, x1, y1, x2, y2, inv_det,
                 &g->dworld_dx.y, &g->dworld_dy.y);
  plane_gradient(v0->world.z, v1->world.z, v2->world.z, x1, y1, x2, y2, inv_det,
                 &g->dworld_dx.z, &g->dworld_dy.z);
  plane_gradient(v0->hden, v1->hden, v2->hden, x1, y1, x2, y2, inv_det,
                 &g->dhden_dx, &g->dhden_dy);
}

/*
 * Walk around a polygon, create the surrounding
 * edges and sort them into the y-bucket.
 */
static void create_edges(View_coord *view_vert, int polygon, Surface *surface,
                         Render_mode render_mode) {
  Edge *edge;
  Edge *first_edge, *last_edge;
  Poly_grad *grad = NULL;
  View_coord *view_ref, *last;
  int nderiv, y1, y2;
  double deltay;
  double x1, x2;
  double hden1, hden2;
  Vector world1, world2;
  Vector norm1, norm2;
  Vector text1, text2;

  view_ref = last = view_vert;
  first_edge = last_edge = NULL;

  if (render_mode == PHONG) {
    grad = (Poly_grad *)arena_alloc(&edge_arena, sizeof(Poly_grad));
    poly_gradients(view_vert, grad);
  }

  do {
    view_ref = view_ref->next;

    /*
     * If we are drawing a line image we dont need
     * to build a complete edgelist. We draw the
     * lines directly instead.
     *
     * Since many lines are drawn twice (edges shared between
     * two polygons) and many line drawing algorithms are unsymmetrical
     * we need to make sure lines are always drawn in the same
     * direction
     */
    if (render_mode == LINE) {
      if (view_ref->view.y < view_ref->next->view.y) {
        (*line_set)(im_data, (int)(view_ref->view.x + 0.5),
                    (int)(view_ref->view.y + 0.5),
                    (int)(view_ref->next->view.x + 0.5),
                    (int)(view_ref->next->view.y + 0.5));
      } else {
        (*line_set)(im_data, (int)(view_ref->next->view.x + 0.5),
                    (int)(view_ref->next->view.y + 0.5),
                    (int)(view_ref->view.x + 0.5),
                    (int)(view_ref->view.y + 0.5));
      }
      continue;
    }

    /*
     * Check if the slope of the edge is positive or negative
     * or zero.
     */
    y1 = (int)(view_ref->view.y + 0.5);
    y2 = (int)(view_ref->next->view.y + 0.5);
    deltay = (double)(y2 - y1);

    if (deltay > 0.0)
      nderiv = 1;
    else if (deltay < 0.0)
      nderiv = -1;
    else
      nderiv = 0;

    /*
     * Check if the edge is horizontal. In that case we
     * just skip it.
     */
    if (nderiv != 0) {

      edge = (Edge *)arena_alloc(&edge_arena, sizeof(Edge));

      x1 = view_ref->view.x;
      x2 = view_ref->next->view.x;
      hden1 = view_ref->hden;
      hden2 = view_ref->next->hden;
      world1 = view_ref->world;
      world2 = view_ref->next->world;
      norm1 = view_ref->normal;
      norm2 = view_ref->next->normal;
      text1 = view_ref->texture;
      text2 = view_ref->next->texture;

      deltay = 1.0 / fabs(deltay);

      if ((reverse_scan && nderiv <= 0) || ((!reverse_scan) && nderiv > 0)) {

        /*
         * The edge has positive slope
         */
        edge->ystart = y2;
        edge->ystop = y1;
        edge->xstart = x2;
        edge->hden = hden2;
        edge->world = world2;
        edge->normal = norm2;
        edge->texture = text2;
        edge->xstep = (x1 - x2) * deltay;
        edge->hdenstep = (hden1 - hden2) * deltay;
        if (render_mode != FLAT) {
          VecComb(edge->normalstep, deltay, norm1, -deltay, norm2);
          VecComb(edge->texturestep, deltay, text1, -deltay, text2);
          if (render_mode == PHONG) {
            VecComb(edge->worldstep, deltay, world1, -deltay, world2);
          }
        }

      } else {

        /*
         * The edge has negative slope.
         */
        edge->ystart = y1;
        edge->ystop = y2;
        edge->xstart = x1;
        edge->hden = hden1;
        edge->world = world1;
        edge->normal = norm1;
        edge->texture = text1;
        edge->xstep = (x2 - x1) * deltay;
        edge->hdenstep = (hden2 - hden1) * deltay;
        if (render_mode != FLAT) {
          VecComb(edge->normalstep, deltay, norm2, -deltay, norm1);
          VecComb(edge->texturestep, deltay, text2, -deltay, text1);
          if (render_mode == PHONG) {
            VecComb(edge->worldstep, deltay, world2, -deltay, world1);
          }
        }
      }
      edge->polygon = polygon;
      edge->surface = surface;
      edge->grad = grad;
      edge->id = edge_count++;
      edge->next = y_bucket[edge->ystart];
      y_bucket[edge->ystart] = edge;

      /*
       * Link all edges of this polygon into a ring, so that an
       * edge can find its polygon's other edges when it becomes
       * active (see active_insert()).
       */
      if (first_edge == NULL) {
        first_edge = edge;
      } else {
        last_edge->sibling = edge;
      }
      last_edge = edge;
    }
  } while (view_ref != last);

  if (last_edge != NULL) {
    last_edge->sibling = first_edge;
  }
}

/*
 * Calculate a new vertex by interpolation between
 * V1 and V2.
 */
static View_coord *interpolate(View_coord *v1, View_coord *v2, double ratio) {
  View_coord *tmp;

  tmp = (View_coord *)arena_alloc(&vcoord_arena, sizeof(View_coord));

  tmp->hden = (1.0 - ratio) * v1->hden + ratio * v2->hden;
  VecComb(tmp->view, 1.0 - ratio, v1->view, ratio, v2->view);
  VecComb(tmp->world, 1.0 - ratio, v1->world, ratio, v2->world);
  VecComb(tmp->normal, 1.0 - ratio, v1->normal, ratio, v2->normal);
  VecComb(tmp->texture, 1.0 - ratio, v1->texture, ratio, v2->texture);
  tmp->next = NULL;

  return tmp;
}

/*
 * Reset the averaged normals of all vertices in the list VREF.
 */
static void reset_normals(Vertex *vref) {
  for (; vref != NULL; vref = vref->next) {
    if (!vref->fixed_normal) {
      MakeVector(vref->normal, 0.0, 0.0, 0.0);
    }
  }
}

/*
 * Clip a polygon using the Sutherland-Hodgeman algorithm for
 * reentrant clipping;
 */

static View_coord *polygon_clip(View_coord *vlist, Clip_plane plane,
                                bool first_vert) {
  static View_coord *first;
  static View_coord *curr;
  View_coord *out1;
  View_coord *out2;
  double curr_limit;
  double first_limit;
  double vlist_limit;
  double ratio;
  bool visible;

  out1 = out2 = NULL;

  if (vlist == NULL) {

    /*
     * Did we get an empty list from the start?
     */
    if (first_vert) {
      return NULL;
    }

    /*
     * Last vertex, close the polygon.
     */
    ratio = 0.0;
    curr_limit = curr->view.z * sipp_current_camera->focal_ratio;
    first_limit = first->view.z * sipp_current_camera->focal_ratio;

    switch (plane) {

    case XMIN:
      if ((curr->view.x < -curr_limit && first->view.x >= -first_limit) ||
          (curr->view.x >= -curr_limit && first->view.x < -first_limit)) {
        ratio = fabs(curr->view.x + curr_limit);
        ratio /= (ratio + fabs(first->view.x + first_limit));
      }
      break;

    case XMAX:
      if ((curr->view.x <= curr_limit && first->view.x > first_limit) ||
          (curr->view.x > curr_limit && first->view.x <= first_limit)) {
        ratio = fabs(curr->view.x - curr_limit);
        ratio /= (ratio + fabs(first->view.x - first_limit));
      }
      break;

    case YMIN:
      if ((curr->view.y < -curr_limit && first->view.y >= -first_limit) ||
          (curr->view.y >= -curr_limit && first->view.y < -first_limit)) {
        ratio = fabs(curr->view.y + curr_limit);
        ratio /= (ratio + fabs(first->view.y + first_limit));
      }
      break;

    case YMAX:
      if ((curr->view.y <= curr_limit && first->view.y > first_limit) ||
          (curr->view.y > curr_limit && first->view.y <= first_limit)) {
        ratio = fabs(curr->view.y - curr_limit);
        ratio /= (ratio + fabs(first->view.y - first_limit));
      }
      break;

    case ZMIN:
      if ((curr->view.z < hither && first->view.z >= hither) ||
          (curr->view.z >= hither && first->view.z < hither)) {
        ratio = fabs(curr->view.z - hither);
        ratio = ratio / (ratio + fabs(first->view.z - hither));
      }
      break;

    case ZMAX:
      if ((curr->view.z <= yon && first->view.z > yon) ||
          (curr->view.z > yon && first->view.z <= yon)) {
        ratio = fabs(curr->view.z - yon);
        ratio = ratio / (ratio + fabs(first->view.z - yon));
      }
      break;
    }

    if (ratio != 0.0) {
      out1 = interpolate(curr, first, ratio);
      return out1;
    } else {
      return NULL;
    }
  }

  vlist_limit = vlist->view.z * sipp_current_camera->focal_ratio;

  if (first_vert) {
    first = vlist;
  } else {
    ratio = 0.0;
    curr_limit = curr->view.z * sipp_current_camera->focal_ratio;

    switch (plane) {

    case XMIN:
      if ((curr->view.x < -curr_limit && vlist->view.x >= -vlist_limit) ||
          (curr->view.x >= -curr_limit && vlist->view.x < -vlist_limit)) {
        ratio = fabs(curr->view.x + curr_limit);
        ratio /= (ratio + fabs(vlist->view.x + vlist_limit));
      }
      break;

    case XMAX:
      if ((curr->view.x <= curr_limit && vlist->view.x > vlist_limit) ||
          (curr->view.x > curr_limit && vlist->view.x <= vlist_limit)) {
        ratio = fabs(curr->view.x - curr_limit);
        ratio /= (ratio + fabs(vlist->view.x - vlist_limit));
      }
      break;

    case YMIN:
      if ((curr->view.y < -curr_limit && vlist->view.y >= -vlist_limit) ||
          (curr->view.y >= -curr_limit && vlist->view.y < -vlist_limit)) {
        ratio = fabs(curr->view.y + curr_limit);
        ratio /= (ratio + fabs(vlist->view.y + vlist_limit));
      }
      break;

    case YMAX:
      if ((curr->view.y <= curr_limit && vlist->view.y > vlist_limit) ||
          (curr->view.y > curr_limit && vlist->view.y <= vlist_limit)) {
        ratio = fabs(curr->view.y - curr_limit);
        ratio /= (ratio + fabs(vlist->view.y - vlist_limit));
      }
      break;

    case ZMIN:
      if ((curr->view.z < hither && vlist->view.z >= hither) ||
          (curr->view.z >= hither && vlist->view.z < hither)) {
        ratio = fabs(curr->view.z - hither);
        ratio = ratio / (ratio + fabs(vlist->view.z - hither));
      }
      break;

    case ZMAX:
      if ((curr->view.z <= yon && vlist->view.z > yon) ||
          (curr->view.z > yon && vlist->view.z <= yon)) {
        ratio = fabs(curr->view.z - yon);
        ratio = ratio / (ratio + fabs(vlist->view.z - yon));
      }
      break;
    }

    if (ratio != 0.0) {
      out1 = interpolate(curr, vlist, ratio);
      out1->next = vlist;
    }
  }

  curr = vlist;
  visible = FALSE;
  switch (plane) {

  case XMIN:
    visible = (curr->view.x >= -vlist_limit);
    break;

  case XMAX:
    visible = (curr->view.x <= vlist_limit);
    break;

  case YMIN:
    visible = (curr->view.y >= -vlist_limit);
    break;

  case YMAX:
    visible = (curr->view.y <= vlist_limit);
    break;

  case ZMIN:
    visible = (curr->view.z >= hither);
    break;

  case ZMAX:
    visible = (curr->view.z <= yon);
    break;
  }

  if (visible) {
    out2 = curr;
    out2->next = polygon_clip(curr->next, plane, FALSE);
    return ((out1) ? (out1) : (out2));

  } else {
    if (out1) {
      out1->next = polygon_clip(curr->next, plane, FALSE);
    } else {
      out1 = polygon_clip(curr->next, plane, FALSE);
    }
    /* vlist is dropped; its memory is reclaimed with vcoord_arena. */
    return out1;
  }
}

/*
 * Call the shader of SURFACE at the vertex V, as FLAT and GOURAUD
 * shading do.  No sample footprint is known there: the derivatives
 * are zero.
 */
static void shade_vertex(Surface *surface, View_coord *v, Color *color,
                         Color *opacity) {
  Shade_point sp;

  memset(&sp, 0, sizeof(sp));
  sp.pos = v->world;
  sp.normal = v->normal;
  sp.texture = v->texture;
  VecSub(sp.view_vec, sipp_current_camera->position, v->world);
  vecnorm(&sp.view_vec);
  (*surface->shader)(&sp, lightsrc_stack, surface->surface, color, opacity);
}

/*
 * Transform vertices into view coordinates. The transform is
 * defined in MATRIX. Store the transformed vertices in a
 * temporary list, create edges in the y_bucket.
 */
static void transf_vertices(Vertex *vertex[], int nvertices, Surface *surface,
                            Transf_mat *view_mat, Transf_mat *tr_mat,
                            double xsiz, double ysiz, Render_mode render_mode) {
  static int polygon = 0; /* incremented for each call to provide */
                          /* unique polygon id numbers */
  View_coord *nhead;
  View_coord *view_ref;
  Color color;
  Color opacity;
  double persp_factor;
  double minsize;
  double tmp;
  int i;

  nhead = NULL;
  minsize = ((xsiz > ysiz) ? ysiz : xsiz);

  /*
   * Everything allocated from vcoord_arena belongs to the previous
   * polygon and is dead by now (create_edges() copies what it needs
   * into the Edges), so start over.
   */
  arena_reset(&vcoord_arena);

  for (i = 0; i < nvertices; i++) {

    view_ref = (View_coord *)arena_alloc(&vcoord_arena, sizeof(View_coord));

    /* Transform the normal (world coordinates) but */
    /* do not include the translation part. */
    view_ref->normal.x = (vertex[i]->normal.x * tr_mat->mat[0][0] +
                          vertex[i]->normal.y * tr_mat->mat[1][0] +
                          vertex[i]->normal.z * tr_mat->mat[2][0]);
    view_ref->normal.y = (vertex[i]->normal.x * tr_mat->mat[0][1] +
                          vertex[i]->normal.y * tr_mat->mat[1][1] +
                          vertex[i]->normal.z * tr_mat->mat[2][1]);
    view_ref->normal.z = (vertex[i]->normal.x * tr_mat->mat[0][2] +
                          vertex[i]->normal.y * tr_mat->mat[1][2] +
                          vertex[i]->normal.z * tr_mat->mat[2][2]);
    vecnorm(&view_ref->normal);

#ifdef FFD
    if (vertex[i]->ffd_vertex == NULL) {
#endif
      /* Transform the vertex to its new world coordinates. */
      point_transform(&view_ref->world, &vertex[i]->pos, tr_mat);

      /* Transform the vertex into view coordinates. */
      point_transform(&view_ref->view, &vertex[i]->pos, view_mat);

      /* Texture coordinates is not affected by transformations. */
      VecCopy(view_ref->texture, vertex[i]->texture);
#ifdef FFD
    } else {
      /* Transform the vertex to its new world coordinates. */
      point_transform(&view_ref->world, &vertex[i]->ffd_vertex->pos, tr_mat);

      /* Transform the vertex into view coordinates. */
      point_transform(&view_ref->view, &vertex[i]->ffd_vertex->pos, view_mat);

      /* Texture coordinates is not affected by transformations. */
      VecCopy(view_ref->texture, vertex[i]->ffd_vertex->texture);
    }
#endif
    view_ref->next = nhead;
    nhead = view_ref;
  }

  /*
   * Clip the resulting polygon. We need to do this
   * before the perpective transformation to keep texture
   * coordinates correct.
   */
  nhead = polygon_clip(nhead, ZMIN, TRUE);
  nhead = polygon_clip(nhead, ZMAX, TRUE);
  if (xsiz > ysiz) {
    tmp = sipp_current_camera->focal_ratio;
    sipp_current_camera->focal_ratio *= xsiz / ysiz;
    nhead = polygon_clip(nhead, XMIN, TRUE);
    nhead = polygon_clip(nhead, XMAX, TRUE);
    sipp_current_camera->focal_ratio = tmp;
    nhead = polygon_clip(nhead, YMIN, TRUE);
    nhead = polygon_clip(nhead, YMAX, TRUE);
  } else {
    tmp = sipp_current_camera->focal_ratio;
    sipp_current_camera->focal_ratio *= ysiz / xsiz;
    nhead = polygon_clip(nhead, YMIN, TRUE);
    nhead = polygon_clip(nhead, YMAX, TRUE);
    sipp_current_camera->focal_ratio = tmp;
    nhead = polygon_clip(nhead, XMIN, TRUE);
    nhead = polygon_clip(nhead, XMAX, TRUE);
  }

  if (nhead == NULL) { /* Nothing left? */
    return;
  }

  /*
   * If we are flat shading, we need a color for the polygon.
   * We call the shader at the first vertex to get this.
   * (This is not quite correct since the normal here is
   * an averaged normal of the surrounding polygons)
   */
  if (render_mode == FLAT) {
    shade_vertex(surface, nhead, &color, &opacity);
  }

  /*
   * Walk around the new (clipped and transformed) polygon and
   * transform it into perspective screen coordinates.
   */
  for (view_ref = nhead;; view_ref = view_ref->next) {
    persp_factor = view_ref->view.z * sipp_current_camera->focal_ratio;
    view_ref->view.x = view_ref->view.x * minsize / persp_factor + xsiz;
    view_ref->view.y = view_ref->view.y * minsize / persp_factor + ysiz;
    view_ref->hden = 1.0 / persp_factor;

    switch (render_mode) {

    /*
     * In PHONG mode we do a homgenous division of texture and
     * world coordinates and store a "homogenous denominator"
     * so we can do correct rational linear interpolation later.
     */
    case PHONG:
      VecScalMul(view_ref->world, view_ref->hden, view_ref->world);
      VecScalMul(view_ref->texture, view_ref->hden, view_ref->texture);
      break;

    /*
     * If we are doing gouraud shading we call the shader at each
     * vertex. Similar homogenous divide are performed on
     * the resulting color and opacity to get correct interpolation
     * later.
     *
     * (FLAT and GOURAUD mode stores color and opacity
     * in the normal and texture vectors, ugly ugly...)
     */
    case GOURAUD: {
      shade_vertex(surface, view_ref, &color, &opacity);
      MakeVector(view_ref->normal, color.red, color.grn, color.blu);
      MakeVector(view_ref->texture, opacity.red, opacity.grn, opacity.blu);
      VecScalMul(view_ref->texture, view_ref->hden, view_ref->texture);
      VecScalMul(view_ref->normal, view_ref->hden, view_ref->normal);
    } break;

    /*
     * In FLAT more we simply store the calculated color and
     * opacity. No divide is necessary since we never interpolate
     * anything here.
     */
    case FLAT:
      MakeVector(view_ref->normal, color.red, color.grn, color.blu);
      MakeVector(view_ref->texture, opacity.red, opacity.grn, opacity.blu);
      break;

    case LINE: /* Handled by create_edges() */
      break;
    }

    /*
     * Last we tie the head and tail together forming a cirkular
     * list, this simplifies edge creation.
     */
    if (view_ref->next == NULL) {
      view_ref->next = nhead;
      break;
    }
  }

  create_edges(nhead, polygon++, surface, render_mode);

  /*
   * The transformed polygon is no longer needed.  Its View_coords
   * live in vcoord_arena and are reclaimed on the next call.
   */
}

#ifdef FFD
/*
 * Perform the user defined free form deformation on
 * all vertices in a surface, creating new temporary
 * vertices in them.
 */
static void do_ffd(Vertex *vertex, Surface *surface) {
  FFD_Vertex *fvp;

  for (; vertex != NULL; vertex = vertex->next) {
    if (vertex->ffd_vertex == NULL) {
      fvp = (FFD_Vertex *)smalloc(sizeof(FFD_Vertex));
    } else {
      fvp = vertex->ffd_vertex;
    }

    fvp->pos = vertex->pos;
    fvp->texture = vertex->texture;

    surface->ffd_func(surface->ffd_data, &vertex->pos, &vertex->texture,
                      &fvp->pos, &fvp->texture);
    vertex->ffd_vertex = fvp;
  }
}

static void ffd_vertices(Surface *surface) {
  if (surface->vertices != NULL) {
    do_ffd(surface->vertices, surface);
  }
}
#endif

/*======================== The scanline sweep =========================*/

/*
 * The object tree traversal produces, for every polygon, a set of
 * immutable Edges bucketed by their first scanline (y_bucket).  The
 * sweep walks the scanlines in scan order keeping an "active list" of
 * the edges crossing the current scanline.  The interpolation state of
 * an active edge (its current x, 1/w, world position, normal and
 * texture coordinates) lives in an Active_edge record which is stepped
 * from scanline to scanline; the Edge itself is never written, so any
 * number of sweeps can share the edges.
 *
 * All state of a sweep is kept in a Render_ctx.  The image is rendered
 * as bands of output scanlines; a band that does not start at the
 * first scanline first brings its active list to the state the
 * sequential sweep would have had there (band_open()).  This is what
 * lets bands be rendered independently, and in parallel.
 */

enum { ACTIVE_ARENA_BLOCK = 64 * 1024 };

typedef struct {
  /* The pass */
  int xres, yres; /* Sub-sampled resolution */
  int oversampl;
  Render_mode render_mode;
  Field field;
  bool step_vectors; /* Step normal/texture/world too */
  bool call_update;  /* Run the update callback here */

  /* Active edges */
  Active_edge *head, *tail;
  Active_edge **by_id;         /* Active record of each Edge, or NULL */
  Active_edge *free_list;      /* Retired records, for reuse */
  Active_edge **retire_bucket; /* Per scanline: records to retire
                                  after it, during a replay */
  Arena arena;                 /* Active_edge records */
  bool analytic;               /* Insertion compares analytic x */

  /* Per-scanline buffers */
  int *pixel_line;   /* Fragment list head per sub-pixel */
  Color **linebuf;   /* oversampl sub-scanlines of colour */
  double **alphabuf; /* ... and of alpha, when alpha is set */
  Pixel_buffer pb;

  /* Output */
  bool alpha;           /* Produce an alpha channel */
  int channels;         /* Bytes per pixel in image: 3 or 4 */
  unsigned char *image; /* Rows of the image, in scan order */
  int xres_out;
  Storage_mode storage_mode;
  bool deliver_rows; /* Call store_line() as rows finish */
} Render_ctx;

/*
 * The replay lists of all bands, see band_lists_build().
 */
typedef struct {
  Edge **edges; /* All lists, one after the other */
  int *start;   /* Index of the first edge of each band ... */
  int *count;   /* ... and its number of edges */
} Band_lists;

/*
 * Work shared by the threads of one scan_and_render().
 */
typedef struct {
  pthread_mutex_t lock;
  int next_band; /* First band nobody has taken yet */
  int nbands;
  int yres_out;
  bool *band_done;  /* Band rendered completely */
  Band_lists lists; /* Replay list of each band */
  float *d_map;     /* Depthmap to render, or NULL for
                       the image */
} Render_job;

/*
 * Scanline bookkeeping.  Scanlines are visited in "scan order": from
 * yres - 1 down to 0 normally, from 0 up when reverse_scan is set.
 */
#define SCAN_STEP (reverse_scan ? 1 : -1)
#define FIRST_SUBLINE(ctx) (reverse_scan ? 0 : (ctx)->yres - 1)
#define OUTLINE_SUBLINE(ctx, k)                                                \
  (reverse_scan ? (k) * (ctx)->oversampl                                       \
                : (ctx)->yres - 1 - (k) * (ctx)->oversampl)
/*
 * The "scanline" number of output line K, as it was passed to
 * store_line() and used for field parity by the original sweep.
 */
#define OUTLINE_NUMBER(ctx, k)                                                 \
  (reverse_scan ? ((ctx)->yres - 1) * (ctx)->oversampl - (k) : (k))

#define PIXEL_OF(x) ((int)((x) + 0.5))

static void store_line(unsigned char *buf, int npixels, int line,
                       Storage_mode storage_mode);

static int edge_retire_line(Edge *edge);

static void render_dmap_line(Render_ctx *ctx, float *dmap_line);

/*
 * x of EDGE at scanline Y, computed rather than stepped.
 */
static double edge_x_at(Edge *edge, int y) {
  return edge->xstart + (double)abs(edge->ystart - y) * edge->xstep;
}

static void ctx_init(Render_ctx *ctx, int xres, int yres, int oversampl,
                     Render_mode render_mode, Field field, int nedges) {
  int i;

  ctx->xres = xres;
  ctx->yres = yres;
  ctx->oversampl = oversampl;
  ctx->render_mode = render_mode;
  ctx->field = field;
  ctx->step_vectors = (render_mode != FLAT);
  ctx->call_update = TRUE;
  ctx->head = ctx->tail = NULL;
  ctx->by_id =
      (Active_edge **)scalloc(nedges > 0 ? nedges : 1, sizeof(Active_edge *));
  ctx->free_list = NULL;
  ctx->retire_bucket = (Active_edge **)scalloc(yres, sizeof(Active_edge *));
  arena_init(&ctx->arena, ACTIVE_ARENA_BLOCK);
  ctx->analytic = FALSE;
  ctx->pixel_line = (int *)scalloc(xres, sizeof(int));
  ctx->linebuf = (Color **)smalloc(oversampl * sizeof(Color *));
  for (i = 0; i < oversampl; i++) {
    ctx->linebuf[i] = (Color *)scalloc(xres, sizeof(Color));
  }
  ctx->alpha = image_alpha;
  ctx->channels = image_alpha ? 4 : 3;
  ctx->alphabuf = NULL;
  if (image_alpha) {
    ctx->alphabuf = (double **)smalloc(oversampl * sizeof(double *));
    for (i = 0; i < oversampl; i++) {
      ctx->alphabuf[i] = (double *)scalloc(xres, sizeof(double));
    }
  }
  pixels_setup(&ctx->pb, xres);
  if (shading_per_pixel && render_mode == PHONG) {
    shade_cache_setup(&ctx->pb, xres / oversampl);
  }
  ctx->image = NULL;
  ctx->xres_out = xres / oversampl;
  ctx->storage_mode = IMAGE_FILE;
  ctx->deliver_rows = FALSE;
}

static void ctx_free(Render_ctx *ctx) {
  int i;

  sfree(ctx->by_id);
  sfree(ctx->retire_bucket);
  arena_release(&ctx->arena);
  sfree(ctx->pixel_line);
  for (i = 0; i < ctx->oversampl; i++) {
    sfree(ctx->linebuf[i]);
  }
  sfree(ctx->linebuf);
  if (ctx->alphabuf != NULL) {
    for (i = 0; i < ctx->oversampl; i++) {
      sfree(ctx->alphabuf[i]);
    }
    sfree(ctx->alphabuf);
  }
  pixels_free(&ctx->pb);
}

/*
 * Drop all active edges, so the context can be used for another band.
 */
static void ctx_clear_active(Render_ctx *ctx) {
  Active_edge *ae;

  for (ae = ctx->head; ae != NULL; ae = ae->next) {
    ctx->by_id[ae->edge->id] = NULL;
  }
  ctx->head = ctx->tail = NULL;
  ctx->free_list = NULL;
  arena_reset(&ctx->arena);
}

static Active_edge *active_alloc(Render_ctx *ctx) {
  Active_edge *ae;

  if (ctx->free_list != NULL) {
    ae = ctx->free_list;
    ctx->free_list = ae->next;
  } else {
    ae = (Active_edge *)arena_alloc(&ctx->arena, sizeof(Active_edge));
  }
  return ae;
}

/*
 * Insert AE into the active list.
 *
 * The list is organized in groups: all active edges of one polygon are
 * adjacent, sorted on x (and on xstep when they round to the same
 * pixel), so that render_scanline() can walk the list in pairs.  Groups
 * are ordered by the time the polygon first became active.
 *
 * To find the group without scanning the list from the head, we look
 * for an active edge among the polygon's siblings (the ring built in
 * create_edges()); polygons are small, so this is a handful of steps.
 * From there we back up to the first edge of the group and place AE
 * with the comparisons the original list-scanning version used.
 *
 * Y is the current scanline.  When ctx->analytic is set (while a band
 * replays the scanlines above it, see band_open()) the x of the edges
 * already in the list is computed for Y instead of read from the
 * (unstepped) records.
 */
static void active_insert(Render_ctx *ctx, Active_edge *ae, int y) {
  Edge *edge = ae->edge;
  Edge *sib;
  Active_edge *ref;
  double ref_x;

  for (sib = edge->sibling; sib != edge && ctx->by_id[sib->id] == NULL;
       sib = sib->sibling)
    ;

  if (sib == edge) {
    ref = NULL;
  } else {
    ref = ctx->by_id[sib->id];
    while (ref->prev != NULL && ref->prev->edge->polygon == edge->polygon) {
      ref = ref->prev;
    }
    while (ref != NULL && ref->edge->polygon == edge->polygon) {
      ref_x = ctx->analytic ? edge_x_at(ref->edge, y) : ref->x;
      if (ref_x > ae->x || (PIXEL_OF(ref_x) == PIXEL_OF(ae->x) &&
                            ref->edge->xstep > edge->xstep)) {
        break;
      }
      ref = ref->next;
    }
  }

  ae->next = ref;
  if (ref == NULL) {
    ae->prev = ctx->tail;
    if (ctx->tail != NULL) {
      ctx->tail->next = ae;
    } else {
      ctx->head = ae;
    }
    ctx->tail = ae;
  } else {
    ae->prev = ref->prev;
    if (ref->prev != NULL) {
      ref->prev->next = ae;
    } else {
      ctx->head = ae;
    }
    ref->prev = ae;
  }
  ctx->by_id[edge->id] = ae;
}

/*
 * Make all edges starting at scanline Y active.
 */
static void active_activate(Render_ctx *ctx, Edge *edge) {
  Active_edge *ae;

  ae = active_alloc(ctx);
  ae->edge = edge;
  ae->y = edge->ystart;
  ae->x = edge->xstart;
  ae->hden = edge->hden;
  ae->world = edge->world;
  ae->normal = edge->normal;
  ae->texture = edge->texture;
  active_insert(ctx, ae, edge->ystart);
  if (ctx->analytic) {
    int line = edge_retire_line(edge);

    ae->retire_next = ctx->retire_bucket[line];
    ctx->retire_bucket[line] = ae;
  }
}

static void active_merge(Render_ctx *ctx, int y) {
  Edge *edge;

  for (edge = y_bucket[y]; edge != NULL; edge = edge->next) {
    active_activate(ctx, edge);
  }
}

/*
 * Take AE out of the active list and put its record on the free list.
 */
static void active_unlink(Render_ctx *ctx, Active_edge *ae) {
  if (ae->prev != NULL) {
    ae->prev->next = ae->next;
  } else {
    ctx->head = ae->next;
  }
  if (ae->next != NULL) {
    ae->next->prev = ae->prev;
  } else {
    ctx->tail = ae->prev;
  }
  ctx->by_id[ae->edge->id] = NULL;
  ae->next = ctx->free_list;
  ctx->free_list = ae;
}

/*
 * After scanline Y: remove the edges that have reached their last
 * scanline.  The order of the remaining edges is unchanged.
 */
static void active_retire(Render_ctx *ctx, int y) {
  Active_edge *ae, *next;

  for (ae = ctx->head; ae != NULL; ae = next) {
    next = ae->next;
    if ((reverse_scan && y >= (ae->edge->ystop - 1)) ||
        ((!reverse_scan) && y <= (ae->edge->ystop + 1))) {
      active_unlink(ctx, ae);
    }
  }
}

/*
 * The scanline after which an edge is retired: its last scanline
 * ystop+1 (ystop-1 scanning upwards), or its first if that comes
 * later.
 */
static int edge_retire_line(Edge *edge) {
  if (reverse_scan) {
    return (edge->ystart > edge->ystop - 1) ? edge->ystart : edge->ystop - 1;
  }
  return (edge->ystart < edge->ystop + 1) ? edge->ystart : edge->ystop + 1;
}

/*
 * Replay version of active_retire(): the records were put into
 * retire buckets by active_merge(), so only the ones due now are
 * touched.  Same result, without walking the list.
 */
static void active_retire_bucket(Render_ctx *ctx, int y) {
  Active_edge *ae, *next;

  for (ae = ctx->retire_bucket[y]; ae != NULL; ae = next) {
    next = ae->retire_next;
    active_unlink(ctx, ae);
  }
  ctx->retire_bucket[y] = NULL;
}

/*
 * Step one active edge to the next scanline.
 */
static void active_step_one(Render_ctx *ctx, Active_edge *ae) {
  Edge *edge = ae->edge;

  ae->y += SCAN_STEP;
  ae->x += edge->xstep;
  ae->hden += edge->hdenstep;
  if (ctx->step_vectors) {
    VecAdd(ae->normal, ae->normal, edge->normalstep);
    VecAdd(ae->texture, ae->texture, edge->texturestep);
    if (ctx->render_mode == PHONG) {
      VecAdd(ae->world, ae->world, edge->worldstep);
    }
  }
}

static void active_step(Render_ctx *ctx) {
  Active_edge *ae;

  for (ae = ctx->head; ae != NULL; ae = ae->next) {
    active_step_one(ctx, ae);
  }
}

/*
 * Prepare CTX to start sweeping at sub-scanline Y0, in the state the
 * sequential sweep would be in when reaching Y0.
 *
 * LIST holds, in activation order, the N edges of every polygon that
 * is still active at Y0 (see band_lists_build()); no other edge can
 * influence where those polygons' groups sit in the active list, so
 * replaying just these with the real activation and retirement logic,
 * but without stepping anything, yields the same order.  The edges
 * still active at Y0 are then stepped to Y0 exactly as the sequential
 * sweep would have stepped them.
 */
static void band_open(Render_ctx *ctx, int y0, Edge **list, int n) {
  Active_edge *ae;
  int y, i;

  ctx_clear_active(ctx);
  if (y0 == FIRST_SUBLINE(ctx)) {
    return;
  }

  ctx->analytic = TRUE;
  i = 0;
  for (y = FIRST_SUBLINE(ctx); y != y0; y += SCAN_STEP) {
    while (i < n && list[i]->ystart == y) {
      active_activate(ctx, list[i++]);
    }
    active_retire_bucket(ctx, y);
  }
  ctx->analytic = FALSE;
  /* Records still in buckets at or after Y0 belong to live edges. */
  for (y = y0; y >= 0 && y < ctx->yres; y += SCAN_STEP) {
    ctx->retire_bucket[y] = NULL;
  }

  for (ae = ctx->head; ae != NULL; ae = ae->next) {
    while (ae->y != y0) {
      active_step_one(ctx, ae);
    }
  }
}

/*
 * Activation order: first scanline in scan order, then position in
 * its y_bucket (edges are pushed on the head of a bucket, so a higher
 * id comes first).
 */
static int edge_activation_cmp(const void *a, const void *b) {
  Edge *ea = *(Edge **)a, *eb = *(Edge **)b;

  if (ea->ystart != eb->ystart) {
    return reverse_scan ? ea->ystart - eb->ystart : eb->ystart - ea->ystart;
  }
  return eb->id - ea->id;
}

/*
 * For every band, collect the edges of the polygons that are still
 * active at the band's first scanline, in activation order.  A polygon
 * is active at a scanline iff its topmost edge starts before it and
 * its bottommost edge ends after it, in scan order.
 *
 * Done once, before the bands are rendered, in O(edges * bands).
 */
static void band_lists_build(Band_lists *bl, Render_ctx *ctx, int nbands,
                             int yres_out) {
  unsigned char *seen;
  Edge *edge, *sib;
  int *y0;
  int band, top, bottom, y, total, n;
  int i;

  y0 = (int *)smalloc(nbands * sizeof(int));
  for (band = 0; band < nbands; band++) {
    y0[band] = OUTLINE_SUBLINE(ctx, (int)((long)yres_out * band / nbands));
  }
  bl->start = (int *)scalloc(nbands, sizeof(int));
  bl->count = (int *)scalloc(nbands, sizeof(int));
  seen = (unsigned char *)scalloc(edge_count > 0 ? edge_count : 1, 1);

  /*
   * Two passes over the polygons: count, then fill.
   */
  for (i = 0; i < 2; i++) {
    if (i == 1) {
      total = 0;
      for (band = 0; band < nbands; band++) {
        bl->start[band] = total;
        total += bl->count[band];
        bl->count[band] = 0;
      }
      bl->edges = (Edge **)smalloc((total > 0 ? total : 1) * sizeof(Edge *));
      memset(seen, 0, edge_count);
    }
    for (y = 0; y < ctx->yres; y++) {
      for (edge = y_bucket[y]; edge != NULL; edge = edge->next) {
        if (seen[edge->id]) {
          continue;
        }
        /* Extent of the polygon, in scan order. */
        top = bottom = edge->ystart;
        n = 0;
        sib = edge;
        do {
          seen[sib->id] = 1;
          n++;
          if (reverse_scan) {
            if (sib->ystart < top)
              top = sib->ystart;
            if (sib->ystop > bottom)
              bottom = sib->ystop;
          } else {
            if (sib->ystart > top)
              top = sib->ystart;
            if (sib->ystop < bottom)
              bottom = sib->ystop;
          }
          sib = sib->sibling;
        } while (sib != edge);

        for (band = 1; band < nbands; band++) {
          /* Active at y0: started before, not yet retired. */
          if (reverse_scan ? (top < y0[band] && y0[band] < bottom)
                           : (top > y0[band] && y0[band] > bottom)) {
            if (i == 0) {
              bl->count[band] += n;
            } else {
              sib = edge;
              do {
                bl->edges[bl->start[band] + bl->count[band]++] = sib;
                sib = sib->sibling;
              } while (sib != edge);
            }
          }
        }
      }
    }
  }
  for (band = 1; band < nbands; band++) {
    qsort(bl->edges + bl->start[band], bl->count[band], sizeof(Edge *),
          edge_activation_cmp);
  }
  sfree(seen);
  sfree(y0);
}

static void band_lists_free(Band_lists *bl) {
  sfree(bl->edges);
  sfree(bl->start);
  sfree(bl->count);
}

/*
 * Interpolate across the active edges on the current scanline and
 * enter the fragments into the pixel buffer.
 */
static void render_scanline(Render_ctx *ctx) {
  Active_edge *startedge, *stopedge;
  int *scanline = ctx->pixel_line;
  Vector worldstep;
  Vector normalstep;
  Vector texturestep;
  double hden, hdenstep;
  double real_z;
  int xstart, xstop;
  double ratio;
  int i;

  startedge = ctx->head;
  stopedge = NULL;

  while (startedge != NULL) {

    stopedge = startedge->next;
    xstart = (int)(startedge->x + 0.5);
    xstop = (int)(stopedge->x - 0.5);
    hden = startedge->hden;

    if (xstart < xstop) {
      ratio = 1.0 / (double)(xstop - xstart);
      hdenstep = (stopedge->hden - hden) * ratio;
      if (ctx->render_mode != FLAT) {
        VecSub(normalstep, stopedge->normal, startedge->normal);
        VecScalMul(normalstep, ratio, normalstep);
        VecSub(texturestep, stopedge->texture, startedge->texture);
        VecScalMul(texturestep, ratio, texturestep);
        if (ctx->render_mode == PHONG) {
          VecSub(worldstep, stopedge->world, startedge->world);
          VecScalMul(worldstep, ratio, worldstep);
        }
      }

    } else {
      hdenstep = 0.0;
      MakeVector(worldstep, 0.0, 0.0, 0.0);
      MakeVector(normalstep, 0.0, 0.0, 0.0);
      MakeVector(texturestep, 0.0, 0.0, 0.0);
    }

    for (i = xstart; i <= xstop; i++) {
      real_z = 1.0 / (sipp_current_camera->focal_ratio * hden);
      scanline[i] = pixel_insert(&ctx->pb, scanline[i], &worldstep,
                                 &texturestep, &normalstep, real_z, hden,
                                 (double)(i - xstart), startedge);
      hden += hdenstep;
    }
    if (ctx->call_update) {
      UPDATE_CALLBACK;
    }
    if (abort_render) {
      return;
    }
    startedge = stopedge->next;
  }
}

/*
 * The alpha of a sub-sample: the largest opacity of its color bands.
 * A single alpha cannot represent a surface that lets different
 * amounts of the three bands through; the largest keeps the straight
 * colors (the premultiplied ones divided by alpha) within range.
 */
static double opacity_max(Color *opacity) {
  double a = opacity->red;

  if (opacity->grn > a) {
    a = opacity->grn;
  }
  if (opacity->blu > a) {
    a = opacity->blu;
  }
  return a;
}

static unsigned char byte_of(double v) {
  if (v <= 0.0) {
    return 0;
  }
  if (v >= 1.0) {
    return 255;
  }
  return (unsigned char)(v * 255.0 + 0.5);
}

/*
 * Turn the sums over the N2 sub-samples of a pixel, SUM of the
 * premultiplied colors and ASUM of alpha, into straight 8 bit RGB and
 * alpha at P.
 */
static void store_rgba(unsigned char *p, Color *sum, double asum, int n2) {
  double alpha = asum / n2;
  double scale = (alpha > 0.0) ? 1.0 / (alpha * n2) : 0.0;

  p[0] = byte_of(sum->red * scale);
  p[1] = byte_of(sum->grn * scale);
  p[2] = byte_of(sum->blu * scale);
  p[3] = byte_of(alpha);
}

/*
 * Clear the fragment lists of a scanline.
 */
static void buffer_clear(int res, int *scanline) {
  int i;

  for (i = 0; i < res; i++) {
    scanline[i] = -1;
  }
}

/*
 * Render output lines K0 (inclusive) to K1 (exclusive) into
 * ctx->image.  Row K of the image holds output line K in scan order.
 */
static void band_render(Render_ctx *ctx, int k0, int k1, Edge **list,
                        int nlist) {
  unsigned char *row;
  Color *col;
  Color opacity;
  Color sum;
  double asum;
  int y, k, curr_line, number;
  int i, j, l;
  int over = ctx->oversampl;
  int n2 = over * over;

  band_open(ctx, OUTLINE_SUBLINE(ctx, k0), list, nlist);
  shadow_jitter_reset();

  y = OUTLINE_SUBLINE(ctx, k0);
  k = k0;
  curr_line = 0;

  while (k < k1 && !abort_render) {

    active_merge(ctx, y);
    number = OUTLINE_NUMBER(ctx, k);

    if (ctx->field == BOTH || (number & 1) == ctx->field) {
      buffer_clear(ctx->xres, ctx->pixel_line);

      /*
       * Interpolate across the polygons and build the fragment
       * lists, then walk the lists front to back, calling the
       * shaders only where the result will be used.
       */
      render_scanline(ctx);

      for (i = 0; i < ctx->xres; i++) {
        col = ctx->linebuf[curr_line] + i;
        if (shading_per_pixel && ctx->render_mode == PHONG) {
          pixel_collect(&ctx->pb, ctx->pixel_line[i], col, &opacity,
                        ctx->render_mode, i / over, over);
        } else {
          pixel_collect(&ctx->pb, ctx->pixel_line[i], col, &opacity,
                        ctx->render_mode, -1, 1);
        }
        if (ctx->alpha) {
          ctx->alphabuf[curr_line][i] = opacity_max(&opacity);
        } else {
          /* Let the background through where the surfaces are not
             opaque. */
          col->red +=
              ((opacity.red >= 1.0) ? 0.0 : 1.0 - opacity.red) * sipp_bgcol.red;
          col->grn +=
              ((opacity.grn >= 1.0) ? 0.0 : 1.0 - opacity.grn) * sipp_bgcol.grn;
          col->blu +=
              ((opacity.blu >= 1.0) ? 0.0 : 1.0 - opacity.blu) * sipp_bgcol.blu;
        }
        if (ctx->call_update) {
          UPDATE_CALLBACK;
        }
        if (abort_render) {
          break;
        }
      }
    }
    if (abort_render) {
      break;
    }

    if (++curr_line == over) {
      if (ctx->field == BOTH || (number & 1) == ctx->field) {
        /*
         * Average the sub-samples into the output row.
         */
        row = ctx->image + (size_t)k * ctx->xres_out * ctx->channels;
        for (i = 0; i < ctx->xres_out; i++) {
          sum.red = sum.grn = sum.blu = 0.0;
          asum = 0.0;
          for (j = i * over; j < i * over + over; j++) {
            for (l = 0; l < over; l++) {
              sum.red += (ctx->linebuf[l] + j)->red;
              sum.grn += (ctx->linebuf[l] + j)->grn;
              sum.blu += (ctx->linebuf[l] + j)->blu;
              if (ctx->alpha) {
                asum += ctx->alphabuf[l][j];
              }
            }
          }
          if (!ctx->alpha) {
            row[i * 3] = (unsigned char)(sum.red / n2 * 255.0 + 0.5);
            row[i * 3 + 1] = (unsigned char)(sum.grn / n2 * 255.0 + 0.5);
            row[i * 3 + 2] = (unsigned char)(sum.blu / n2 * 255.0 + 0.5);
          } else {
            store_rgba(row + i * 4, &sum, asum, n2);
          }
        }
        pixels_reinit(&ctx->pb);
        shade_cache_clear(&ctx->pb);
        if (ctx->deliver_rows) {
          store_line(row, ctx->xres_out, number, ctx->storage_mode);
        }
      }
      curr_line = 0;
      k++;
    }

    active_retire(ctx, y);
    active_step(ctx);
    y += SCAN_STEP;
  }
}

/*
 * Depthmap counterpart of band_render(): scanlines K0 (inclusive) to
 * K1 (exclusive) of D_MAP.  Each position keeps its smallest depth, so
 * the result cannot depend on how the map is divided into bands.
 */
static void depthmap_band(Render_ctx *ctx, int k0, int k1, Edge **list,
                          int nlist, float *d_map) {
  int y, k;

  band_open(ctx, OUTLINE_SUBLINE(ctx, k0), list, nlist);
  y = OUTLINE_SUBLINE(ctx, k0);

  for (k = k0; k < k1 && !abort_render; k++) {
    active_merge(ctx, y);
    render_dmap_line(ctx, d_map + (depthmap_size - 1 - y) * depthmap_size);
    active_retire(ctx, y);
    active_step(ctx);
    y += SCAN_STEP;
  }
}

/*
 * Deliver one finished output row: to the image file, or to the
 * user's pixel function.
 */
static void store_line(unsigned char *buf, int npixels, int line,
                       Storage_mode storage_mode) {
  int channels = image_alpha ? 4 : 3;
  int i, j;

  switch (storage_mode) {
  case IMAGE_FILE:
    if (image_format == IMAGE_PNG) {
      /* PNG is written when all rows are known, see file_end(). */
      if (png_nrows < png_lines) {
        memcpy(png_rows + (size_t)png_nrows * png_rowbytes, buf, png_rowbytes);
        png_nrows++;
      }
    } else {
      fwrite(buf, sizeof(unsigned char), (size_t)npixels * channels,
             image_file);
      fflush(image_file);
    }
    break;

  case FUNCTION:
    for (i = 0, j = 0; j < npixels; j++, i += channels) {
      (*pixel_set)(im_data, j, line, buf[i], buf[i + 1], buf[i + 2],
                   (channels == 4) ? buf[i + 3] : 255);
    }
    break;

  default:
    break;
  }
}

/*
 * The number of lines an image of YRES_OUT lines has when only FIELD
 * of it is rendered.
 */
static int field_lines(int yres_out, Field field) {
  switch (field) {
  case EVEN:
    return (yres_out & 1) ? (yres_out >> 1) + 1 : yres_out >> 1;
  case ODD:
    return yres_out >> 1;
  default:
    return yres_out;
  }
}

/*
 * Begin the image file: write the header of a PPM or PAM file, or set
 * up the row buffer of a PNG file.
 */
static void file_begin(int xres_out, int yres_out, Field field) {
  int nlines = field_lines(yres_out, field);
  size_t nbytes;

  if (image_format == IMAGE_PNG) {
    png_rowbytes = xres_out * (image_alpha ? 4 : 3);
    png_lines = nlines;
    png_nrows = 0;
    nbytes = (size_t)png_rowbytes * nlines;
    png_rows = (unsigned char *)scalloc(nbytes > 0 ? nbytes : 1, 1);
    return;
  }

  fprintf(image_file, image_alpha ? "P7\n" : "P6\n");
  fprintf(image_file, "#Image rendered with SIPP %s\n", SIPP_VERSION);
  switch (field) {
  case EVEN:
    fprintf(image_file, "#Image field containing EVEN lines\n");
    break;
  case ODD:
    fprintf(image_file, "#Image field containing ODD lines\n");
    break;
  case BOTH:
    break;
  }
  if (image_alpha) {
    fprintf(image_file,
            "WIDTH %d\nHEIGHT %d\nDEPTH 4\nMAXVAL 255\n"
            "TUPLTYPE RGB_ALPHA\nENDHDR\n",
            xres_out, nlines);
  } else {
    fprintf(image_file, "%d\n%d\n255\n", xres_out, nlines);
  }
}

/*
 * Finish the image file.  Rows that were never delivered (after an
 * abort) are left black, or transparent.
 */
static void file_end(int xres_out) {
  if (image_format == IMAGE_PNG) {
    sipp_png_write(image_file, xres_out, png_lines, image_alpha ? 4 : 3,
                   png_rows);
    sfree(png_rows);
    png_rows = NULL;
  }
}

/*
 * Render the whole image: split it into bands, render them, and
 * deliver the rows in scan order.
 */
typedef struct {
  Render_job *job;
  Render_ctx *ctx;
} Worker_arg;

static void *worker_main(void *arg) {
  Worker_arg *wa = (Worker_arg *)arg;
  Render_job *job = wa->job;
  int band, k0, k1;

  for (;;) {
    pthread_mutex_lock(&job->lock);
    band = job->next_band++;
    pthread_mutex_unlock(&job->lock);
    if (band >= job->nbands || abort_render) {
      break;
    }
    k0 = (int)((long)job->yres_out * band / job->nbands);
    k1 = (int)((long)job->yres_out * (band + 1) / job->nbands);
    if (job->d_map != NULL) {
      depthmap_band(wa->ctx, k0, k1, job->lists.edges + job->lists.start[band],
                    job->lists.count[band], job->d_map);
    } else {
      band_render(wa->ctx, k0, k1, job->lists.edges + job->lists.start[band],
                  job->lists.count[band]);
    }
    if (!abort_render) {
      job->band_done[band] = TRUE;
    }
  }
  return NULL;
}

/*
 * How many threads and bands to use for NLINES output lines.
 * Several bands per thread even out the differences in cost between
 * bands.  SIPP_BANDS in the environment overrides the number of bands;
 * it exists for testing the band logic.
 */
static void choose_bands(int nlines, int *nthreads, int *nbands) {
  int t, b;

  t = render_threads;
  if (t > nlines) {
    t = nlines;
  }
  if (t < 1) {
    t = 1;
  }
  b = (t > 1) ? t * 4 : 1;
  if (getenv("SIPP_BANDS") != NULL) {
    b = atoi(getenv("SIPP_BANDS"));
  }
  if (b < 1) {
    b = 1;
  }
  if (b > nlines) {
    b = nlines;
  }
  *nthreads = t;
  *nbands = b;
}

/*
 * Render NBANDS bands of NLINES output lines with NTHREADS threads,
 * using the initialized contexts CTX[0..NTHREADS-1].  D_MAP selects
 * depthmap rendering; NULL renders the image.  On return
 * job->band_done tells which bands were completed.
 */
static void run_bands(Render_job *job, Render_ctx *ctx, int nthreads,
                      int nbands, int nlines, float *d_map) {
  Worker_arg *args;
  pthread_t *threads;
  int t;

  args = (Worker_arg *)smalloc(nthreads * sizeof(Worker_arg));
  threads = (pthread_t *)smalloc(nthreads * sizeof(pthread_t));
  for (t = 0; t < nthreads; t++) {
    args[t].job = job;
    args[t].ctx = &ctx[t];
  }

  pthread_mutex_init(&job->lock, NULL);
  job->next_band = 0;
  job->nbands = nbands;
  job->yres_out = nlines;
  job->band_done = (bool *)scalloc(nbands, sizeof(bool));
  job->d_map = d_map;
  band_lists_build(&job->lists, &ctx[0], nbands, nlines);

  for (t = 1; t < nthreads; t++) {
    if (pthread_create(&threads[t], NULL, worker_main, &args[t]) != 0) {
      fprintf(stderr, "sipp: cannot create rendering thread\n");
      exit(1);
    }
  }
  worker_main(&args[0]); /* This thread works too */
  for (t = 1; t < nthreads; t++) {
    pthread_join(threads[t], NULL);
  }
  pthread_mutex_destroy(&job->lock);
  band_lists_free(&job->lists);
  sfree(threads);
  sfree(args);
}

/*
 * Render the whole image: split it into bands, render them (with
 * several threads if asked to), and deliver the rows in scan order.
 */
static void scan_and_render(int xres, int yres, Storage_mode storage_mode,
                            Render_mode render_mode, int oversampl,
                            Field field) {
  Render_job job;
  Render_ctx *ctx;
  unsigned char *image;
  int xres_out = xres / oversampl;
  int yres_out = yres / oversampl;
  int nthreads, nbands, t, k, number;

  if (storage_mode == IMAGE_FILE) {
    file_begin(xres_out, yres_out, field);
  }

  choose_bands(yres_out, &nthreads, &nbands);

  image = (unsigned char *)scalloc(
      (size_t)xres_out * yres_out * (image_alpha ? 4 : 3), 1);

  ctx = (Render_ctx *)smalloc(nthreads * sizeof(Render_ctx));
  for (t = 0; t < nthreads; t++) {
    ctx_init(&ctx[t], xres, yres, oversampl, render_mode, field, edge_count);
    ctx[t].image = image;
    ctx[t].storage_mode = storage_mode;
    /*
     * Only the calling thread may run the user's callbacks.  With
     * a single thread the rows are also delivered as they finish,
     * exactly as before.
     */
    ctx[t].call_update = (t == 0);
    ctx[t].deliver_rows = (nthreads == 1);
  }
  run_bands(&job, ctx, nthreads, nbands, yres_out, NULL);
  for (t = 0; t < nthreads; t++) {
    ctx_free(&ctx[t]);
  }

  /*
   * With several threads the rows are delivered now, in scan order.
   * After an abort only complete bands are delivered.
   */
  if (nthreads > 1) {
    int band, k_done;

    for (band = 0; band < nbands && job.band_done[band]; band++)
      ;
    k_done = (int)((long)yres_out * band / nbands);
    for (k = 0; k < k_done; k++) {
      number = OUTLINE_NUMBER(&ctx[0], k);
      if (field == BOTH || (number & 1) == field) {
        store_line(image + (size_t)k * xres_out * ctx[0].channels, xres_out,
                   number, storage_mode);
      }
    }
  }
  if (storage_mode == IMAGE_FILE) {
    file_end(xres_out);
  }
  sfree(job.band_done);
  sfree(ctx);
  sfree(image);
}

/*
 * Enter the depths along the current scanline into DMAP_LINE, keeping
 * the smallest depth seen at each position.
 */
static void render_dmap_line(Render_ctx *ctx, float *dmap_line) {
  Active_edge *startedge, *stopedge;
  double hden, hdenstep;
  float zfact;
  float real_z;
  int xstart, xstop;
  int i;

  startedge = ctx->head;
  stopedge = NULL;

  zfact = (float)(1.0 / sipp_current_camera->focal_ratio);

  while (startedge != NULL && !abort_render) {

    stopedge = startedge->next;
    xstart = (int)(startedge->x + 0.5);
    xstop = (int)(stopedge->x + 0.5);
    hden = startedge->hden;

    if (xstart < xstop) {
      hdenstep = (stopedge->hden - hden) / (double)(xstop - xstart);
    } else {
      hdenstep = 0.0;
    }

    for (i = xstart; i <= xstop; i++) {
      real_z = zfact / hden;
      if (real_z < dmap_line[i]) {
        dmap_line[i] = real_z;
      }
      hden += hdenstep;
    }
    startedge = stopedge->next;
    if (ctx->call_update) {
      UPDATE_CALLBACK;
    }
  }
}

/*
 * Sweep the y_bucket of a lightsource, writing a depthmap.
 */
static void scan_depthmap(float *d_map) {
  Render_job job;
  Render_ctx *ctx;
  int nthreads, nbands, t;

  choose_bands(depthmap_size, &nthreads, &nbands);
  if (getenv("SIPP_DMAP_BANDS") != NULL) { /* For testing */
    nbands = atoi(getenv("SIPP_DMAP_BANDS"));
    if (nbands < 1)
      nbands = 1;
    if (nbands > depthmap_size)
      nbands = depthmap_size;
  }
  ctx = (Render_ctx *)smalloc(nthreads * sizeof(Render_ctx));
  for (t = 0; t < nthreads; t++) {
    ctx_init(&ctx[t], depthmap_size, depthmap_size, 1, PHONG, BOTH, edge_count);
    ctx[t].step_vectors = FALSE; /* Only x and 1/w are needed */
    ctx[t].call_update = (t == 0);
  }
  run_bands(&job, ctx, nthreads, nbands, depthmap_size, d_map);
  for (t = 0; t < nthreads; t++) {
    ctx_free(&ctx[t]);
  }
  sfree(job.band_done);
  sfree(ctx);
}

/*
 * Push the current transformation matrix on the matrix stack.
 */
static void matrix_push(void) {
  struct tm_stack_t *new_tm;

  new_tm = (struct tm_stack_t *)smalloc(sizeof(struct tm_stack_t));
  MatCopy(&new_tm->mat, &curr_mat);
  new_tm->next = tm_stack;
  tm_stack = new_tm;
}

/*
 * Pop the top of the matrix stack and make
 * it the new current transformation matrix.
 */
static void matrix_pop(void) {
  struct tm_stack_t *tmp;

  MatCopy(&curr_mat, &tm_stack->mat);
  tmp = tm_stack;
  tm_stack = tm_stack->next;
  sfree(tmp);
}

/*
 * Traverse an object hierarchy, transform each object
 * according to its transformation matrix.
 * Transform all polygons in the object to view coordinates.
 * Build the edge lists in y_bucket.
 */
static void traverse_object_tree(Object *object, Transf_mat *view_mat, int xres,
                                 int yres, Render_mode render_mode) {
  Surface *surfref;
  Polygon *polyref;
  Vector eyepoint, tmp;
  Transf_mat loc_view_mat;
  double m[3][4], dtmp;
  int i, j, surfidx;

  if (object == NULL) {
    return;
  }

  matrix_push();
  mat_mul(&curr_mat, &object->transf, &curr_mat);
  mat_mul(&loc_view_mat, &curr_mat, view_mat);

  VecCopy(tmp, sipp_current_camera->position);

  /*
   * Do an inverse transformation of the viewpoint to use
   * when doing backface culling (in calc_normals()).
   */
  tmp.x -= curr_mat.mat[3][0];
  tmp.y -= curr_mat.mat[3][1];
  tmp.z -= curr_mat.mat[3][2];
  m[0][0] = curr_mat.mat[0][0];
  m[0][1] = curr_mat.mat[1][0];
  m[0][2] = curr_mat.mat[2][0];
  m[0][3] = tmp.x;
  m[1][0] = curr_mat.mat[0][1];
  m[1][1] = curr_mat.mat[1][1];
  m[1][2] = curr_mat.mat[2][1];
  m[1][3] = tmp.y;
  m[2][0] = curr_mat.mat[0][2];
  m[2][1] = curr_mat.mat[1][2];
  m[2][2] = curr_mat.mat[2][2];
  m[2][3] = tmp.z;

  /*
   * Solve the 3x3 linear equation system.
   */
  if (m[0][0] == 0.0) {
    if (m[1][0] != 0.0)
      j = 1;
    else
      j = 2;
    for (i = 0; i < 4; i++) {
      dtmp = m[0][i];
      m[0][i] = m[j][i];
      m[j][i] = dtmp;
    }
  }

  for (j = 1; j < 3; j++) {
    m[j][0] /= (-m[0][0]);
    for (i = 1; i < 4; i++)
      m[j][i] += m[0][i] * m[j][0];
  }

  if (m[1][1] == 0.0)
    for (i = 1; i < 4; i++) {
      dtmp = m[1][i];
      m[1][i] = m[2][i];
      m[2][i] = dtmp;
    }

  if (m[1][1] != 0.0) {
    m[2][1] /= (-m[1][1]);
    m[2][2] += m[1][2] * m[2][1];
    m[2][3] += m[1][3] * m[2][1];
  }

  eyepoint.z = m[2][3] / m[2][2];
  eyepoint.y = (m[1][3] - eyepoint.z * m[1][2]) / m[1][1];
  eyepoint.x =
      (m[0][3] - eyepoint.z * m[0][2] - eyepoint.y * m[0][1]) / m[0][0];

  for (surfidx = 0; surfidx < object->num_surfaces; surfidx++) {
    surfref = object->surfaces[surfidx];
#ifdef FFD
    if (surfref->ffd_func != NULL) {
      ffd_vertices(surfref);
    }
#endif
    calc_normals(surfref->polygons, eyepoint);

    for (polyref = surfref->polygons; polyref != NULL;
         polyref = polyref->next) {

      if (!polyref->backface) {
        transf_vertices(polyref->vertex, polyref->nvertices, surfref,
                        &loc_view_mat, &curr_mat, (double)xres / 2.0,
                        (double)yres / 2.0, render_mode);
        UPDATE_CALLBACK;
        if (abort_render) {
          matrix_pop();
          return;
        }
      }
    }

    reset_normals(surfref->vertices);
  }

  for (i = 0; i < object->num_sub_objs; i++) {
    traverse_object_tree(object->sub_objs[i], view_mat, xres, yres,
                         render_mode);
  }

  matrix_pop();
}

/*
 * Render depthmaps for the lightsources that will cast
 * shadows. Place the camera in the position of each lightsource
 * in turn and render the depthmap. Store the matrix converting
 * from world coordinates to depthmap coordinates together with
 * the depthmap.
 */
void shadowmaps_create(int size) {
  Transf_mat view_mat;
  Vector view_vec;
  Vector tmpv;
  double tmp;
  Lightsource *lp;
  Camera *tmp_camera;
  Camera *light_camera;
  bool backface_tmp;

  depthmap_size = size;
  depthmaps_create();

  y_bucket = (Edge **)scalloc(depthmap_size, sizeof(Edge *));

  tmp_camera = sipp_current_camera;
  light_camera = camera_create();
  *light_camera = *sipp_current_camera;
  camera_clipping(light_camera, 0.0, 0.0); /* Not the camera's planes */
  backface_tmp = show_backfaces;
  show_backfaces = TRUE;

  for (lp = lightsrc_stack; lp != NULL && !abort_render; lp = lp->next) {
    if (lp->shadow.active) {
      VecCopy(light_camera->position, ((Spot_light_info *)(lp->info))->pos);
      VecCopy(light_camera->lookat, ((Spot_light_info *)(lp->info))->point);
      light_camera->focal_ratio = lp->shadow.fov_factor;
      camera_use(light_camera);
      VecSub(view_vec, light_camera->position, light_camera->lookat);

      /*
       * Build an up-vector which is always perpendicular
       * to the view vector. Rotate the components of the view
       * vector and if the vector is along the (1 1 1) line we
       * simply negate the x-component. The result is cross multiplied
       * with the view vector to get the up vector.
       */
      VecCopy(tmpv, view_vec);
      vecnorm(&tmpv);
      tmp = tmpv.x;
      tmpv.x = tmpv.y;
      tmpv.y = tmpv.z;
      tmpv.z = tmp;
      if (fabs(tmpv.x - tmpv.y) < 1e-5 && fabs(tmpv.x - tmpv.z) < 1e-5 &&
          fabs(tmpv.z - tmpv.y) < 1e-5) {

        tmpv.x = -tmpv.x;
      }
      VecCross(light_camera->up, tmpv, view_vec);

      lp->shadow.bias = VecLen(view_vec) * 0.005;
      get_view_transf(&view_mat, light_camera, PHONG);
      lp->shadow.matrix = view_mat;

      MatCopy(&curr_mat, &ident_matrix);

      arena_reset(&edge_arena);
      edge_count = 0;
      traverse_object_tree(sipp_world, &view_mat, depthmap_size - 1,
                           depthmap_size - 1, PHONG);

      scan_depthmap(lp->shadow.d_map);
    }
  }

  camera_use(tmp_camera);
  camera_destruct(light_camera);
  show_backfaces = backface_tmp;

  if (abort_render) {
  }

  sfree(y_bucket);
}

/*
 * Destoy all the shadowmaps created by shadowmaps_create()
 */
void shadowmaps_destruct(void) { depthmaps_destruct(); }

/*
 * "Main" functions in rendering. Allocate y-bucket, transform vertices
 * into viewing coordinates, make edges and sort them into the y-bucket.
 * Call scan_and_render to do the real work.
 */
static void render_main(int xres, int yres, Storage_mode storage_mode,
                        Render_mode render_mode, int oversampling,
                        Field field) {
  Transf_mat view_mat;

  get_view_transf(&view_mat, sipp_current_camera, render_mode);

  abort_render = FALSE;
  switch (render_mode) {
  case LINE:
    if (storage_mode == IMAGE_FILE) {
      im_data = sipp_bitmap_create(xres, yres);
      line_set = bitmap_line;
    }
    break;

  case FLAT:
  case GOURAUD:
  case PHONG:
    if (auto_shadows) {
      shadowmaps_create(depthmap_size);
    }
    xres *= oversampling;
    yres *= oversampling;
    y_bucket = (Edge **)scalloc(yres, sizeof(Edge *));
  }

  MatCopy(&curr_mat, &ident_matrix);
  arena_reset(&edge_arena);
  edge_count = 0;
  traverse_object_tree(sipp_world, &view_mat, xres - 1, yres - 1, render_mode);

  switch (render_mode) {
  case LINE:
    if (storage_mode == IMAGE_FILE) {
      if (image_format == IMAGE_PNG) {
        sipp_bitmap_write_png(image_file, im_data);
      } else {
        sipp_bitmap_write(image_file, im_data);
      }
      sipp_bitmap_destruct(im_data);
    }
    break;

  case FLAT:
  case GOURAUD:
  case PHONG:
    if (!abort_render) {
      scan_and_render(xres, yres, storage_mode, render_mode, oversampling,
                      field);
    }
    sfree(y_bucket);
    if (auto_shadows) {
      shadowmaps_destruct();
    }
    break;
  }

  /*
   * Give the memory used by edges and transformed vertices back.
   * (Keeping the blocks around would save a few mallocs per frame in
   * an animation, but this keeps the memory footprint between
   * renderings identical to what it was before.)
   */
  arena_release(&edge_arena);
  arena_release(&vcoord_arena);
}

/*
 * Take the file format and the alpha flag out of FORMAT (see sipp.h).
 * Returns FALSE, after a message, if the format is unknown.
 */
static bool set_format(const char *caller, Image_format format) {
  image_format = (Image_format)(format & ~IMAGE_ALPHA);
  image_alpha = (format & IMAGE_ALPHA) != 0;
  if (image_format != IMAGE_PPM && image_format != IMAGE_PNG) {
    fprintf(stderr, "%s: unknown image format %d\n", caller, format);
    return FALSE;
  }
  return TRUE;
}

void render_image_file(int xres, int yres, FILE *im_file, Image_format format,
                       Render_mode render_mode, int oversampling) {
  if (!set_format("render_image_file", format)) {
    return;
  }
  image_file = im_file;
  render_main(xres, yres, IMAGE_FILE, render_mode, oversampling, BOTH);
}

void render_image_func(int xres, int yres, Pixel_func *pixel_func, void *data,
                       Image_format format, Render_mode render_mode,
                       int oversampling) {
  if (!set_format("render_image_func", format)) {
    return;
  }
  im_data = data;
  pixel_set = pixel_func;
  line_set = (Line_func *)(void (*)(void))pixel_func; /* LINE mode only */
  render_main(xres, yres, FUNCTION, render_mode, oversampling, BOTH);
}

void render_field_file(int xres, int yres, FILE *im_file, Image_format format,
                       Render_mode render_mode, int oversampling, Field field) {
  if (render_mode == LINE) {
    fprintf(stderr, "render_field_file: Can't render line fields\n");
    return;
  }
  if (!set_format("render_field_file", format)) {
    return;
  }
  image_file = im_file;
  render_main(xres, yres, IMAGE_FILE, render_mode, oversampling, field);
}

void render_field_func(int xres, int yres, Pixel_func *pixel_func, void *data,
                       Image_format format, Render_mode render_mode,
                       int oversampling, Field field) {
  if (render_mode == LINE) {
    fprintf(stderr, "render_field_func: Can't render line fields\n");
    return;
  }
  if (!set_format("render_field_func", format)) {
    return;
  }
  im_data = data;
  pixel_set = pixel_func;
  render_main(xres, yres, FUNCTION, render_mode, oversampling, field);
}

/*
 * The customary file name extension of an image rendered with FORMAT
 * in RENDER_MODE: "ppm", "pbm", "pam" or "png".
 */
const char *sipp_image_extension(Image_format format, Render_mode render_mode) {
  if ((format & ~IMAGE_ALPHA) == IMAGE_PNG) {
    return "png";
  }
  if (render_mode == LINE) {
    return "pbm";
  }
  return (format & IMAGE_ALPHA) ? "pam" : "ppm";
}

/*============= Functions that handles global initializations==============*/

/*
 * Function to terminate rendering prematurely.
 */
void sipp_render_terminate(void) { abort_render = TRUE; }

/*
 * If the argument is TRUE, render the scanlines in reverse.
 */
void sipp_render_direction(Scan_direction direction) {
  reverse_scan = (direction == BOTTOM_TO_TOP);
}

/*
 * Save a function pointer to call during the rendering process.  This function
 * is used to handle X-windows updates, etc.  Period is the frequence of that
 * the update is called.  The unit of period is every pixel rendered and units
 * of work that take a similar amount of time.  This is very approimate.
 * Call with proc NULL to disable updates.
 */
void sipp_set_update_callback(Update_func *func, void *client_data,
                              int period) {
  update_func = func;
  update_client_data = client_data;
  update_period = period;
}

/*
 * If called with TRUE as argument, no backface culling will
 * be performed. If a polygon is backfacing it will be rendered
 * as facing in the opposit direction.
 */
void sipp_show_backfaces(bool flag) { show_backfaces = flag; }

/*
 * If called with TRUE, objects will cast shadows. The second
 * argument is then used as the size of the depthmaps.
 */
/*
 * Choose between shading every sub-sample (FLAG = FALSE, the default)
 * and shading each polygon once per output pixel (FLAG = TRUE) when
 * rendering with oversampling.  With FLAG = TRUE the shader is called
 * once per polygon per pixel and the result is reused for all of the
 * pixel's sub-samples; edges are still anti-aliased by the oversampling,
 * but shading detail within a polygon (e.g. procedural textures) is
 * sampled once per pixel.  Only affects PHONG rendering.
 */
/*
 * Set the number of threads used to render an image.  N <= 0 selects
 * the number of processors available.  The default is 1.
 */
void sipp_render_threads(int n) {
  if (n <= 0) {
#ifdef _SC_NPROCESSORS_ONLN
    n = (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
    n = 1;
#endif
  }
  render_threads = (n > 0) ? n : 1;
}

void sipp_shading_per_pixel(bool flag) { shading_per_pixel = flag; }

void sipp_shadows(bool flag, int size) {
  if ((auto_shadows = flag) == TRUE) {
    if (size != 0) {
      depthmap_size = size;
    } else {
      depthmap_size = 256;
    }
  }
}

/*
 * Set the background color of the image.
 */
void sipp_background(double red, double grn, double blu) {
  sipp_bgcol.red = red;
  sipp_bgcol.grn = grn;
  sipp_bgcol.blu = blu;
}

/*
 * Necessary initializations.
 */
void sipp_init(void) {
  arena_init(&edge_arena, EDGE_ARENA_BLOCK);
  arena_init(&vcoord_arena, VCOORD_ARENA_BLOCK);
  objects_init();
  lightsource_init();
  camera_init();
  sipp_shadows(FALSE, 0);
  sipp_shading_per_pixel(FALSE);
  sipp_render_threads(1);
  sipp_show_backfaces(FALSE);
  sipp_render_direction(TOP_TO_BOTTOM);
  sipp_background(0.0, 0.0, 0.0);
  sipp_set_update_callback(NULL, NULL, 0);
}
