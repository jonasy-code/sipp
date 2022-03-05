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

#include <stdio.h>
#include <sys/types.h>
#ifndef NOMEMCPY
#include <memory.h>
#endif

#include <math.h>
#include <values.h>

#ifndef MAXFLOAT
#   define MAXFLOAT ((float)3.40282346638528860e+38)
#endif

#include <xalloca.h>
#include <sipp.h>
#include <smalloc.h>

#include <lightsource.h>
#include <geometric.h>
#include <rendering.h>
#include <pixelbuf.h>
#include <objects.h>
#include <sipp_bitmap.h>
#include <viewpoint.h>

#include "patchlevel.h"
char *SIPP_VERSION = VERSION;

/*
 * Static global variables.
 */
static bool          show_backfaces;  /* Don't do backface culling */
static bool          reverse_scan;    /* Render scan lines in reverse */
static bool          auto_shadows;    /* Calculate shadows */
static Edge        **y_bucket;        /* Y-bucket for edge lists. */
static FILE         *image_file;      /* File to store image in      */
                                      /* when rendering into a file. */
static void        (*pixel_set)();    /* Pointer to function for setting  */
                                      /* a pixel when rendering with user */
                                      /* defined function.                */
static void         *im_data;         /* Data to pixel_set() */

       int           depthmap_size;   /* Size of the depthmaps */


/*
 * Flag that can be set to TRUE to terminate the rendering.
 */
static bool          abort_render;

/*
 * Function to call during rendering process to check for windown events, etc,
 */
static Update_func  *update_func            = NULL;
static void         *update_client_data     = NULL;
static int           update_period          = 0;
static int           update_count           = 0;
    
/*
 * This macro is used to handle the calls to the update call back.  The
 * count is updated and the update proc is called when the period is reached.
 * The approimate unit of the count is the amount of time it takes to render
 * one pixel.
 */
#define UPDATE_CALLBACK \
    if ((update_func != NULL) && (++update_count > update_period)) { \
        update_count = 0; \
        (*update_func) (update_client_data); \
    }
    

/*
 * Stack of transformation matrices used
 * when traversing an object hierarchy.
 */
static struct tm_stack_t {
    Transf_mat         mat;
    struct tm_stack_t *next;
} *tm_stack;

static Transf_mat      curr_mat;     /* Current transformation matrix */

/*
 * Prototypes of internal functions.
 */
static void
calc_normals _ANSI_ARGS_((Polygon *pstart,
                          Vector   eyepoint));

static void
create_edges _ANSI_ARGS_((View_coord *view_vert,
                          int         polygon,
                          Surface    *surface,
                          int         render_mode));

static void
clean_up_y_bucket _ANSI_ARGS_((int size));

static View_coord *
interpolate _ANSI_ARGS_((View_coord *v1,
                         View_coord *v2,
                         double      ratio));

static void
reset_normals _ANSI_ARGS_((Vertex *vref));

static View_coord *
polygon_clip _ANSI_ARGS_((View_coord *vlist,
                          int         plane,
                          bool        first_vert));

static void
transf_vertices _ANSI_ARGS_((Vertex     *vertex[],
                             int         nvertices,
                             Surface    *surface,
                             Transf_mat *view_mat,
                             Transf_mat *tr_mat,
                             double      xsiz,
                             double      ysiz,
                             int         render_mode));

#ifdef FFD
static void
do_ffd _ANSI_ARGS_((Vertex  *vertex,
                    Surface *surface));

static void
ffd_vertices _ANSI_ARGS_((Surface  *surface));
#endif

static void
render_scanline _ANSI_ARGS_((int      res,
                             int     *scanline,
                             Edge    *edge_list,
                             int      render_mode));

static Edge *
insert_edge _ANSI_ARGS_((Edge *edge_list,
                         Edge *edge));

static Edge *
merge_edge_lists _ANSI_ARGS_((Edge *list1,
                              Edge *list2));

static void
store_line _ANSI_ARGS_((u_char  *buf,
                        int      npixels,
                        int      line,
                        int      storage_mode));

static void
buffer_clear _ANSI_ARGS_((int    res,
                          int   *scanline));

static void
scan_and_render _ANSI_ARGS_((int   xres,
                             int   yres,
                             int   storage_mode,
                             int   render_mode,
                             int   oversampl,
                             int   field));

static void
matrix_push _ANSI_ARGS_((void));

static void
matrix_pop _ANSI_ARGS_((void));

static void
traverse_object_tree _ANSI_ARGS_((Object      *object,
                                  Transf_mat  *view_mat,
                                  int          xres, 
                                  int          yres,
                                  int          render_mode));

static void
render_dmap_line _ANSI_ARGS_((float       *dmap_line,
                              Edge        *edge_list));


static void
scan_depthmap _ANSI_ARGS_((float      *d_map));

static void
render_main _ANSI_ARGS_((int   xres, 
                         int   yres,
                         int   storage_mode,
                         int   render_mode,
                         int   oversampling,
                         int   field));


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
static void
calc_normals(pstart, eyepoint)
    Polygon *pstart;    /* Head of polygon list */
    Vector   eyepoint;  /* Viewpoint transformed to local coordinate system */
{
    Polygon    *polyref;
    Vertex    **vlist;
    Vector      normal;
    int         i, j;
    double      plane_const;


    for (polyref = pstart; polyref != NULL; polyref = polyref->next) {

        vlist = polyref->vertex;
        MakeVector(normal, 0.0, 0.0, 0.0);

        for (i = 0; i < polyref->nvertices; i++) {
            j = (i + 1) % polyref->nvertices;
#ifdef FFD
            if (vlist[i]->ffd_vertex == NULL) {                
#endif 
                normal.x += ((vlist[i]->pos.y - vlist[j]->pos.y)
                             * (vlist[i]->pos.z + vlist[j]->pos.z));
                normal.y += ((vlist[i]->pos.z - vlist[j]->pos.z)
                             * (vlist[i]->pos.x + vlist[j]->pos.x));
                normal.z += ((vlist[i]->pos.x - vlist[j]->pos.x)
                             * (vlist[i]->pos.y + vlist[j]->pos.y));
#ifdef FFD
            } else {
                normal.x += ((vlist[i]->ffd_vertex->pos.y 
                              - vlist[j]->ffd_vertex->pos.y)
                             * (vlist[i]->ffd_vertex->pos.z 
                                + vlist[j]->ffd_vertex->pos.z));
                normal.y += ((vlist[i]->ffd_vertex->pos.z 
                              - vlist[j]->ffd_vertex->pos.z)
                             * (vlist[i]->ffd_vertex->pos.x 
                                + vlist[j]->ffd_vertex->pos.x));
                normal.z += ((vlist[i]->ffd_vertex->pos.x 
                              - vlist[j]->ffd_vertex->pos.x)
                             * (vlist[i]->ffd_vertex->pos.y 
                                + vlist[j]->ffd_vertex->pos.y));
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
 * Walk around a polygon, create the surrounding
 * edges and sort them into the y-bucket.
 */
static void
create_edges(view_vert, polygon, surface, render_mode)
    View_coord *view_vert;
    int         polygon;
    Surface    *surface;
    int         render_mode;
{
    Edge       *edge;
    View_coord *view_ref, *last;
    int         nderiv, y1, y2;
    double      deltay;
    double      x1, x2;
    double      hden1, hden2;
    Vector      world1, world2;
    Vector      norm1, norm2;
    Vector      text1, text2;


    view_ref = last = view_vert;

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
                (*pixel_set)(im_data, 
                             (int)(view_ref->view.x + 0.5), 
                             (int)(view_ref->view.y + 0.5),
                             (int)(view_ref->next->view.x + 0.5), 
                             (int)(view_ref->next->view.y + 0.5));
            } else {
                (*pixel_set)(im_data, 
                             (int)(view_ref->next->view.x + 0.5), 
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

            edge = (Edge *)smalloc(sizeof(Edge));

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

            if ((reverse_scan    && nderiv <= 0) ||
                ((!reverse_scan) && nderiv  > 0)) {

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
                        VecComb(edge->worldstep, deltay, world1, 
                                -deltay, world2);
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
                        VecComb(edge->worldstep, deltay, world2, 
                                -deltay, world1);
                    }
                }
            }
            edge->polygon = polygon;
            edge->surface = surface;
            edge->next = y_bucket[edge->ystart];
            y_bucket[edge->ystart] = edge;
        }
    } while (view_ref != last);
}



/*
 * Used to clean up edges in y_bucket when rendering is terminated 
 * prematurely
 */
static void
clean_up_y_bucket (size)
    int size;
{
    int   y;
    Edge *edge, *edgefree;

    for (y = 0; y < size; y++) {
        for (edge = y_bucket [y]; edge != NULL;) {
            edgefree = edge;
            edge = edgefree->next;
            sfree(edgefree);
        }
    }
}



/*
 * Calculate a new vertex by interpolation between
 * V1 and V2.
 */
static View_coord *
interpolate(v1, v2, ratio)
    View_coord *v1, *v2;
    double      ratio;
{
    View_coord *tmp;

    tmp = (View_coord *)smalloc(sizeof(View_coord));

    tmp->hden = (1.0 - ratio) * v1->hden + ratio * v2->hden;
    VecComb(tmp->view, 1.0 - ratio, v1->view, ratio, v2->view);
    VecComb(tmp->world, 1.0 - ratio, v1->world, ratio, v2->world);
    VecComb(tmp->normal, 1.0 - ratio, v1->normal, ratio, v2->normal);
    VecComb(tmp->texture, 1.0 - ratio, v1->texture, ratio, v2->texture);
    tmp->next = NULL;

    return tmp;
}



/*
 * Reset the averaged normals in the vertex tree P.
 */
static void
reset_normals(vref)
    Vertex *vref;
{
    if (vref != NULL) {
        if (!vref->fixed_normal) {
            MakeVector(vref->normal, 0.0, 0.0, 0.0);
        }
        reset_normals(vref->big);
        reset_normals(vref->sml);
    }
}



/*
 * Clip a polygon using the Sutherland-Hodgeman algorithm for
 * reentrant clipping;
 */
#define XMIN 0
#define XMAX 1
#define YMIN 2
#define YMAX 3
#define ZMIN 4
#define ZMAX 5

static View_coord *
polygon_clip(vlist, plane, first_vert)
    View_coord *vlist;
    int         plane;
    bool        first_vert;
{
    static View_coord   *first;
    static View_coord   *curr;
    View_coord          *out1;
    View_coord          *out2;
    double               curr_limit;
    double               first_limit;
    double               vlist_limit;
    double               ratio;
    bool                 visible;

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
            if ((curr->view.x < -curr_limit 
                 && first->view.x >= -first_limit)
                || (curr->view.x >= -curr_limit 
                    && first->view.x < -first_limit)) {
                ratio = fabs(curr->view.x + curr_limit);
                ratio /= (ratio + fabs(first->view.x + first_limit)); 
            }
            break;

          case XMAX:
            if ((curr->view.x <= curr_limit 
                 && first->view.x > first_limit)
                || (curr->view.x > curr_limit 
                    && first->view.x <= first_limit)) {
                ratio = fabs(curr->view.x - curr_limit);
                ratio /= (ratio + fabs(first->view.x - first_limit));
            }
            break;

          case YMIN:
            if ((curr->view.y < -curr_limit 
                 && first->view.y >= -first_limit)
                || (curr->view.y >= -curr_limit 
                    && first->view.y < -first_limit)) {
                ratio = fabs(curr->view.y + curr_limit);
                ratio /= (ratio + fabs(first->view.y + first_limit));
            }
            break;

          case YMAX:
            if ((curr->view.y <= curr_limit 
                 && first->view.y > first_limit)
                || (curr->view.y > curr_limit 
                    && first->view.y <= first_limit)) {
                ratio = fabs(curr->view.y - curr_limit);
                ratio /= (ratio + fabs(first->view.y - first_limit));
            }
            break;

          case ZMIN:
            if ((curr->view.z < hither 
                 && first->view.z >= hither)
                || (curr->view.z >= hither 
                    && first->view.z < hither)) {
                ratio = fabs(curr->view.z - hither);
                ratio = ratio / (ratio + fabs(first->view.z - hither));
            }
            break;

          case ZMAX:
            if ((curr->view.z <= yon 
                 && first->view.z > yon)
                || (curr->view.z > yon 
                    && first->view.z <= yon)) {
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
            if ((curr->view.x < -curr_limit 
                 && vlist->view.x >= -vlist_limit)
                || (curr->view.x >= -curr_limit 
                    && vlist->view.x < -vlist_limit)) {
                ratio = fabs(curr->view.x + curr_limit);
                ratio /= (ratio + fabs(vlist->view.x + vlist_limit));
            }
            break;

          case XMAX:
            if ((curr->view.x <= curr_limit 
                 && vlist->view.x > vlist_limit)
                || (curr->view.x > curr_limit 
                    && vlist->view.x <= vlist_limit)) {
                ratio = fabs(curr->view.x - curr_limit);
                ratio /= (ratio + fabs(vlist->view.x - vlist_limit));
            }
            break;

          case YMIN:
            if ((curr->view.y < -curr_limit 
                 && vlist->view.y >= -vlist_limit)
                || (curr->view.y >= -curr_limit 
                    && vlist->view.y < -vlist_limit)) {
                ratio = fabs(curr->view.y + curr_limit);
                ratio /= (ratio + fabs(vlist->view.y + vlist_limit));
            }
            break;

          case YMAX:
            if ((curr->view.y <= curr_limit 
                 && vlist->view.y > vlist_limit)
                || (curr->view.y > curr_limit 
                    && vlist->view.y <= vlist_limit)) {
                ratio = fabs(curr->view.y - curr_limit);
                ratio /= (ratio + fabs(vlist->view.y - vlist_limit));
            }
            break;

          case ZMIN:
            if ((curr->view.z < hither 
                 && vlist->view.z >= hither)
                || (curr->view.z >= hither 
                    && vlist->view.z < hither)) {
                ratio = fabs(curr->view.z - hither);
                ratio = ratio / (ratio + fabs(vlist->view.z - hither));
            }
            break;

          case ZMAX:
            if ((curr->view.z <= yon 
                 && vlist->view.z > yon)
                || (curr->view.z > yon 
                    && vlist->view.z <= yon)) {
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
        sfree(vlist);
        return out1;
    }
}



/*
 * Transform vertices into view coordinates. The transform is
 * defined in MATRIX. Store the transformed vertices in a
 * temporary list, create edges in the y_bucket.
 */
static void
transf_vertices(vertex, nvertices, surface, view_mat, tr_mat, 
                xsiz, ysiz, render_mode)
    Vertex     *vertex[];
    int         nvertices;
    Surface    *surface;
    Transf_mat *view_mat;
    Transf_mat *tr_mat;
    double      xsiz, ysiz;
    int         render_mode;
{
    static int  polygon = 0;        /* incremented for each call to provide */
                                    /* unique polygon id numbers */
    View_coord *nhead;
    View_coord *view_ref;
    View_coord *mark;
    Color       color;                    
    Color       opacity;
    double      persp_factor;
    double      minsize;
    double      tmp;
    int         i;

    nhead = NULL;
    minsize = ((xsiz > ysiz) ? ysiz : xsiz);

    for (i = 0; i < nvertices; i++) {

        view_ref = (View_coord *)smalloc(sizeof(View_coord));

        /* Transform the normal (world coordinates) but */
        /* do not include the translation part. */
        view_ref->normal.x = (vertex[i]->normal.x * tr_mat->mat[0][0]
                              + vertex[i]->normal.y * tr_mat->mat[1][0]
                              + vertex[i]->normal.z * tr_mat->mat[2][0]);
        view_ref->normal.y = (vertex[i]->normal.x * tr_mat->mat[0][1]
                              + vertex[i]->normal.y * tr_mat->mat[1][1]
                              + vertex[i]->normal.z * tr_mat->mat[2][1]);
        view_ref->normal.z = (vertex[i]->normal.x * tr_mat->mat[0][2]
                              + vertex[i]->normal.y * tr_mat->mat[1][2]
                              + vertex[i]->normal.z * tr_mat->mat[2][2]);
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
            point_transform(&view_ref->world, 
                            &vertex[i]->ffd_vertex->pos, tr_mat);

            /* Transform the vertex into view coordinates. */
            point_transform(&view_ref->view, 
                            &vertex[i]->ffd_vertex->pos, view_mat);

            /* Texture coordinates is not affected by transformations. */
            VecCopy(view_ref->texture, 
                    vertex[i]->ffd_vertex->texture);
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

    if (nhead == NULL) {    /* Nothing left? */
        return;
    }


    
    /*
     * If we are flat shading, we need a color for the polygon.
     * We call the shader at the first vertex to get this.
     * (This is not quite correct since the normal here is
     * an averaged normal of the surrounding polygons)
     */
    if (render_mode == FLAT) {
        Vector view_vec;

        VecSub(view_vec, sipp_current_camera->position, nhead->world);
        vecnorm(&view_vec);
        (*surface->shader)
            (&nhead->world, &nhead->normal, &nhead->texture, 
             &view_vec, lightsrc_stack, surface->surface, 
             &color, &opacity);
    }


    /*
     * Walk around the new (clipped and transformed) polygon and 
     * transform it into perspective screen coordinates.
     */
    for (view_ref = nhead;; view_ref = view_ref->next) {
        persp_factor = view_ref->view.z * sipp_current_camera->focal_ratio; 
        view_ref->view.x  = view_ref->view.x * minsize / persp_factor + xsiz;
        view_ref->view.y  = view_ref->view.y * minsize / persp_factor + ysiz;
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
          case GOURAUD: 
            {
                Vector view_vec;
            
                VecSub(view_vec, sipp_current_camera->position, 
                       view_ref->world);
                vecnorm(&view_vec);
                (*surface->shader)
                    (&view_ref->world, &view_ref->normal, &view_ref->texture, 
                     &view_vec, lightsrc_stack, surface->surface, 
                     &color, &opacity);
                MakeVector(view_ref->normal, color.red, color.grn, color.blu);
                MakeVector(view_ref->texture, 
                           opacity.red, opacity.grn, opacity.blu);
                VecScalMul(view_ref->texture, view_ref->hden, 
                           view_ref->texture);
                VecScalMul(view_ref->normal, view_ref->hden, view_ref->normal);
            }
            break;

          /*
           * In FLAT more we simply store the calculated color and 
           * opacity. No divide is necessary since we never interpolate
           * anything here.
           */
          case FLAT:
            MakeVector(view_ref->normal, color.red, color.grn, color.blu);
            MakeVector(view_ref->texture, 
                       opacity.red, opacity.grn, opacity.blu);
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
     * Free the memory used by the transformed polygon.
     */
    mark = nhead;
    do {
        view_ref = nhead;
        nhead = nhead->next;
        sfree(view_ref);
    } while (nhead != mark);
}



#ifdef FFD
/*
 * Perform the user defined free form deformation on
 * all vertices in a surface, creating new temporary
 * vertices in them.
 */
static void
do_ffd(vertex, surface)
    Vertex  *vertex;
    Surface *surface;
{
    FFD_Vertex *fvp;
    
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
    if (vertex->big != NULL) {
        do_ffd(vertex->big, surface);
    }
    if (vertex->sml != NULL) {
        do_ffd(vertex->sml, surface);
    }
}

static void
ffd_vertices(surface)
    Surface  *surface;
{
    if (surface->vertices != NULL) {
        do_ffd(surface->vertices, surface);
    }
}
#endif 



/*
 * Read edge pairs from the edge list EDGE_LIST. Walk along the scanline
 * and interpolate z value, world coordinates, texture coordinates and 
 * normal vector as we go. Store info about each pixel in the pixel buffer.
 */
static void
render_scanline(res, scanline, edge_list, render_mode)
    int      res;
    int     *scanline;
    Edge    *edge_list;
    int      render_mode;
{
    Edge  *startedge, *stopedge;
    Vector worldstep;
    Vector normalstep;
    Vector texturestep;
    double hden, hdenstep;
    double real_z;
    int    xstart, xstop;
    double ratio;
    int    i;
    
    startedge = edge_list;
    stopedge = NULL;

    while (startedge != NULL) {

        stopedge = startedge->next;
        xstart = (int)(startedge->xstart + 0.5);
        xstop  = (int)(stopedge->xstart - 0.5);
        hden = startedge->hden;
 
        if (xstart < xstop) {
            ratio = 1.0 / (double)(xstop - xstart);
            hdenstep = (stopedge->hden - hden) * ratio;
            if (render_mode != FLAT) {
                VecSub(normalstep, stopedge->normal, startedge->normal);
                VecScalMul(normalstep, ratio, normalstep);
                VecSub(texturestep, stopedge->texture, startedge->texture);
                VecScalMul(texturestep, ratio, texturestep);
                if (render_mode == PHONG) {
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
            scanline[i] = pixel_insert(scanline[i], &worldstep, &texturestep, 
                                       &normalstep, real_z, hden, 
                                       (double)(i - xstart), startedge); 
            hden += hdenstep;
        }
        UPDATE_CALLBACK;
        if (abort_render) {
            return;
        }
        startedge = stopedge->next;
    }
}




/*
 * Insert an edge into an edge list. Edges belonging to the same
 * polygon must be inserted sorted in x, so that edge pairs are
 * created.
 */
static Edge *
insert_edge(edge_list, edge)
    Edge *edge_list, *edge;
{
    Edge *edge_ref1;
    Edge *edge_ref2;

    /*
     * If list is empty, just insert the edge.
     */
    if (edge_list == NULL) {
	edge->next = NULL;
        return edge;
    }

    /*
     * If the edges to our polygon is first in the list, check
     * if our edge should be inserted first.
     */
    if (edge_list->polygon == edge->polygon) {
        if (edge_list->xstart > edge->xstart) {
            edge->next = edge_list;
            return edge;
        } else if ((((int)(edge_list->xstart + 0.5))
                    == ((int)(edge->xstart + 0.5)))
                   && (edge_list->xstep > edge->xstep)) {
            edge->next = edge_list;
            return edge;
        }
    }

    /*
     * Check if our polygon is in the list at all.
     */
    edge_ref1 = edge_list;
    edge_ref2 = edge_list->next;
    if (edge_ref1->polygon != edge->polygon) {
        while (edge_ref2 != NULL && edge_ref2->polygon != edge->polygon) {
            edge_ref1 = edge_ref2;
            edge_ref2 = edge_ref2->next;
        }
    }

    /*
     * Insert the edge at the right place,  sorted in x if our 
     * polygon was found, otherwize last in the list.
     */
    while (1) {
        if (edge_ref2 == NULL) {
            edge->next = edge_ref2;
            edge_ref1->next = edge;
            break;
        } else if ((edge_ref2->polygon != edge->polygon)
                   || ((edge_ref2->xstart > edge->xstart)
                       || ((((int)(edge_ref2->xstart + 0.5)) 
                            == ((int)(edge->xstart + 0.5)))
                           && (edge_ref2->xstep > edge->xstep)))) {
            edge->next = edge_ref2;
            edge_ref1->next = edge;
            break;
        } else {
            edge_ref1 = edge_ref2;
            edge_ref2 = edge_ref2->next;
        }
    }

    return edge_list;
}
        


/*
 * Merge two edge lists.
 */
static Edge *
merge_edge_lists(list1, list2)
    Edge *list1, *list2;
{
    Edge *eref1, *eref2, *next;
    
    if (list2 == NULL)
        return list1;

    eref1 = list1;
    eref2 = list2;
    do {
        next = eref2->next;
        eref1 = insert_edge(eref1, eref2);
	eref2 = next;
    } while (eref2 != NULL);

    return eref1;
}



/*
 * Store a rendered line on the place indicated by STORAGE_MODE.
 */
static void
store_line(buf, npixels, line, storage_mode)
    u_char  *buf;
    int      npixels;
    int      line;
    int      storage_mode;
{
    int i, j;

    switch (storage_mode) {
      case PPM_FILE:
        fwrite(buf, sizeof(u_char), npixels * 3, image_file);
        fflush(image_file);
        break;

      case FUNCTION:
        for (i = 0, j = 0; j < npixels; j++, i += 3) {
            (*pixel_set)(im_data, j, line, buf[i], buf[i + 1], buf[i + 2]);
        }
        break;

      default:
        break;
    }
}


static void
buffer_clear(res, scanline)
    int    res;
    int   *scanline;
{
    int         i;
    
    for (i = 0; i < res; i++) {
        scanline[i] = -1;
    }
}

    

/*
 * Allocate the needed buffers. Create a list of active edges and
 * move down the y-bucket, inserting and deleting edges from this
 * active list as we go. Call render_scanline for each scanline and
 * then do a second pass through the pixelbuffer to do the actual shading.
 * This scheme saves a lot of unnecessary shader calls.
 * Last we do an average filtering before storing the scanline.
 */
static void
scan_and_render(xres, yres, storage_mode, render_mode, oversampl, field)
    int   xres, yres;
    int   storage_mode;
    int   render_mode;
    int   oversampl;
    int   field;
{
    Edge         *active_list;
    Edge         *edgep, *edgep2;
    int          *pixel_line;
    Color       **linebuf;
    u_char       *line;
    int           curr_line;
    int           scanline;
    int           y, next_edge, y_limit;
    Color         sum;
    int           i, j, k;
    
    line = (u_char *)smalloc(xres * 3 * sizeof(u_char));
    pixel_line = (int *)scalloc(xres, sizeof(int));
    linebuf  = (Color **)alloca(oversampl * sizeof(Color *));
    for (i = 0; i < oversampl; i++) {
        linebuf[i] = (Color *)scalloc(xres, sizeof(Color));
    }
    pixels_setup(xres);


    if (storage_mode == PPM_FILE) {
        fprintf(image_file, "P6\n");
        fprintf(image_file, "#Image rendered with SIPP %s\n", SIPP_VERSION);

        switch (field) {
          case BOTH:
            fprintf(image_file, "%d\n%d\n255\n", xres / oversampl, 
                                                 yres / oversampl);
            break;

          case EVEN:
            fprintf(image_file, "#Image field containing EVEN lines\n");
            fprintf(image_file, "%d\n%d\n255\n", xres / oversampl, 
                    ((yres / oversampl) & 1)
                    ? ((yres / oversampl) >> 1) + 1
                    :  (yres / oversampl) >> 1);
            break;

          case ODD:
            fprintf(image_file, "#Image field containing ODD lines\n");
            fprintf(image_file, "%d\n%d\n255\n", xres / oversampl, 
                                                 (yres / oversampl) >> 1);
            break;
        }
    }

    if (reverse_scan) {
        y = 0;
        y_limit = yres;
        scanline =  (yres - 1) * oversampl;
    } else {
        y = yres - 1;
        y_limit = -1;
        scanline =  0;
    }
    active_list = NULL;
    curr_line = 0;

    /*
     * The abort_render flag maybe set in render_scanline.
     */
    while (y != y_limit && !abort_render) {

        active_list = merge_edge_lists(active_list, y_bucket[y]);
        y_bucket[y] = NULL;

        if (reverse_scan) {
            next_edge = y + 1;
            while (next_edge < y_limit && y_bucket[next_edge] == NULL)
                next_edge++;
        } else {
            next_edge = y - 1;
            while (next_edge >= 0 && y_bucket[next_edge] == NULL)
                next_edge--;
        }
        while ((reverse_scan    && (y < next_edge)) ||
               ((!reverse_scan) && (y > next_edge))) {
            if (field == BOTH || (scanline & 1) == field) {
                buffer_clear(xres, pixel_line);

                /*
                 * Here we call the routine to perform interpolation
                 * across the polygons and build the information in the
                 * pixel buffer.
                 */
                render_scanline(xres, pixel_line, active_list, render_mode); 

                /*
                 * Now we do a second pass through the pixel buffer. The
                 * information is now depth-sorted so shaders
                 * (which are called inside pixel_collect() are only called
                 * if the result will actually be used.
                 */
                for (i = 0; i < xres; i++) {
                    pixel_collect(pixel_line[i], linebuf[curr_line] + i, 
                                  render_mode);
                    UPDATE_CALLBACK;
                    if (abort_render) {
                        break;
                    }
                }
            }

            if (abort_render) {
                y_bucket[y] = active_list;  /* Save for later cleanup */
                break;
            }

            if (++curr_line == oversampl) {

                if (field == BOTH || (scanline & 1) == field) {
                    /*
                     * Average the pixel.
                     */
                    for (i = 0; i < ((xres / oversampl)); i++) {
                        sum.red = 0.0;
                        sum.grn = 0.0;
                        sum.blu = 0.0;
                        for (j = i * oversampl;                          
                             j < (i * oversampl + oversampl); j++) {
                            for (k = 0; k < oversampl; k++) {
                                sum.red += (linebuf[k] + j)->red;
                                sum.grn += (linebuf[k] + j)->grn;
                                sum.blu += (linebuf[k] + j)->blu;
                            }
                        }
                        line[i * 3]    = (u_char)(sum.red 
                                                  / (oversampl * oversampl) 
                                                  * 255.0 + 0.5);
                        line[i * 3 + 1] = (u_char)(sum.grn
                                                   / (oversampl * oversampl) 
                                                   * 255.0 + 0.5);
                        line[i * 3 + 2] = (u_char)(sum.blu 
                                                   / (oversampl * oversampl) 
                                                   * 255.0 + 0.5);
                    }
                    store_line(line, xres / oversampl, scanline, 
                               storage_mode);
                    pixels_reinit();
                }

                curr_line = 0;
                scanline += reverse_scan ? -1 : 1;
            }
            
	    if (active_list != NULL) {
                
	        edgep2 = active_list;
	        edgep = active_list->next;
	        while (edgep != NULL) {
	            if ((reverse_scan &&
                         edgep->ystart >= (edgep->ystop - 1)) ||
                        ((!reverse_scan) &&
                         edgep->ystart <= (edgep->ystop + 1))) {
                        edgep2->next = edgep->next;
		        sfree(edgep);
	                edgep = edgep2->next;
		    } else {
		        edgep2 = edgep;
		        edgep = edgep->next;
		    }
                }
                
  	        if ((reverse_scan && 
                     active_list->ystart >= (active_list->ystop - 1)) ||
                    ((!reverse_scan) &&
                     active_list->ystart <= (active_list->ystop + 1))) {
	            edgep = active_list;
		    active_list = active_list->next;
	            sfree(edgep);
	        }
                
	        edgep = active_list;
	        while (edgep != NULL) {
	            edgep->ystart += reverse_scan ? 1 : -1;
		    edgep->xstart += edgep->xstep;
                    edgep->hden += edgep->hdenstep;
                    if (render_mode != FLAT) {
                        VecAdd(edgep->normal, edgep->normal,
                               edgep->normalstep); 
                        VecAdd(edgep->texture, edgep->texture,
                               edgep->texturestep); 
                        if (render_mode == PHONG) {
                            VecAdd(edgep->world, edgep->world, 
                                   edgep->worldstep);
                        }
                    }
		    edgep = edgep->next;
	        }
	    }
            y += reverse_scan ? 1 : -1;
	}
    }
    sfree(line);
    sfree(pixel_line);
    for (i = 0; i < oversampl; i++) {
        sfree(linebuf[i]);
    }
    pixels_free();
}



/*
 * Push the current transformation matrix on the matrix stack.
 */
static void
matrix_push()
{
    struct tm_stack_t *new_tm;

    new_tm = (struct tm_stack_t *)smalloc(sizeof(struct tm_stack_t));
    MatCopy(&new_tm->mat, &curr_mat);
    new_tm->next = tm_stack;
    tm_stack     = new_tm;
}


/*
 * Pop the top of the matrix stack and make
 * it the new current transformation matrix.
 */
static void
matrix_pop()
{
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
static void
traverse_object_tree(object, view_mat, xres, yres, render_mode)
    Object      *object;
    Transf_mat  *view_mat;
    int          xres, yres;
    int          render_mode;
{
    Surface     *surfref;
    Polygon     *polyref;
    Vector       eyepoint, tmp;
    Transf_mat   loc_view_mat;
    double       m[3][4], dtmp;
    int          i, j, surfidx;


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
    m[0][0] = curr_mat.mat[0][0] ; m[0][1] = curr_mat.mat[1][0];
    m[0][2] = curr_mat.mat[2][0] ; m[0][3] = tmp.x;
    m[1][0] = curr_mat.mat[0][1] ; m[1][1] = curr_mat.mat[1][1];
    m[1][2] = curr_mat.mat[2][1] ; m[1][3] = tmp.y;
    m[2][0] = curr_mat.mat[0][2] ; m[2][1] = curr_mat.mat[1][2];
    m[2][2] = curr_mat.mat[2][2] ; m[2][3] = tmp.z;
    
    /*
     * Solve the 3x3 linear equation system.
     */
    if (m[0][0] == 0.0) {
        if (m[1][0] != 0.0)
            j = 1;
        else
            j = 2;
        for (i = 0; i < 4; i++) {
            dtmp     = m[0][i];
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
            dtmp     = m[1][i];
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
    eyepoint.x = (m[0][3] - eyepoint.z * m[0][2] 
                  - eyepoint.y * m[0][1]) / m[0][0];

    for (surfidx = 0; surfidx < object->num_surfaces; surfidx++) {
        surfref = object->surfaces [surfidx];
#ifdef FFD
        if (surfref->ffd_func != NULL) {
            ffd_vertices(surfref);
        }
#endif         
        calc_normals(surfref->polygons, eyepoint);

        for (polyref = surfref->polygons; polyref != NULL; 
             polyref = polyref->next) {

            if (!polyref->backface) {
                transf_vertices(polyref->vertex, polyref->nvertices, 
                                surfref, &loc_view_mat, &curr_mat, 
                                (double)xres / 2.0, (double)yres / 2.0, 
                                render_mode);
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
 * Render one scanline in a depth-map. This is just a stripped
 * version of render_scanline().
 */
static void
render_dmap_line(dmap_line, edge_list)
    float       *dmap_line;
    Edge        *edge_list;
{
    Edge  *startedge, *stopedge;
    double hden, hdenstep;
    float  zfact;
    float  real_z;
    double ratio;
    int    xstart, xstop;
    int    i;
    
    startedge = edge_list;
    stopedge = NULL;

    zfact = (float)(1.0 / sipp_current_camera->focal_ratio);

    while (startedge != NULL && !abort_render) {

        stopedge = startedge->next;
        xstart = (int)(startedge->xstart + 0.5);
        xstop  = (int)(stopedge->xstart + 0.5);
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
        UPDATE_CALLBACK;
    }
}



/*
 * Similar function to scan_and_render() used when rendering depthmaps.
 * Allocate the needed buffers. Create a list of active edges and
 * move down the y-bucket, inserting and deleting edges from this
 * active list as we go. Call render_dmap_line() for each scanline.
 */
static void
scan_depthmap(d_map)
    float      *d_map;
{
    Edge            *active_list;
    Edge            *edgep, *edgep2;
    int              y, next_edge, y_limit;
    
    if (reverse_scan) {
        y = 0;
        y_limit = depthmap_size;
    } else {
        y = depthmap_size - 1;
        y_limit = -1;
    }
    active_list = NULL;
 
    while (y != y_limit) {

        active_list = merge_edge_lists(active_list, y_bucket[y]);
        y_bucket[y] = NULL;

        if (reverse_scan) {
            next_edge = y + 1;
            while (next_edge < y_limit && y_bucket[next_edge] == NULL)
                next_edge++;
        } else {
            next_edge = y - 1;
            while (next_edge >= 0 && y_bucket[next_edge] == NULL)
                next_edge--;
        }

        while ((reverse_scan    && (y < next_edge)) ||
               ((!reverse_scan) && (y > next_edge))) {

            render_dmap_line(d_map + (depthmap_size - 1 - y) * depthmap_size, 
                             active_list); 

	    if (active_list != NULL) {
                
	        edgep2 = active_list;
	        edgep = active_list->next;
	        while (edgep != NULL) {
	            if ((reverse_scan &&
                         edgep->ystart >= (edgep->ystop - 1)) ||
                        ((!reverse_scan) &&
                         edgep->ystart <= (edgep->ystop + 1))) {
                        edgep2->next = edgep->next;
		        sfree(edgep);
	                edgep = edgep2->next;
		    } else {
		        edgep2 = edgep;
		        edgep = edgep->next;
		    }
                }
                
  	        if ((reverse_scan && 
                     active_list->ystart >= (active_list->ystop - 1)) ||
                    ((!reverse_scan) &&
                     active_list->ystart <= (active_list->ystop + 1))) {
	            edgep = active_list;
		    active_list = active_list->next;
	            sfree(edgep);
	        }
                
	        edgep = active_list;
	        while (edgep != NULL) {
	            edgep->ystart += reverse_scan ? 1 : -1;
		    edgep->xstart += edgep->xstep;
                    edgep->hden += edgep->hdenstep;
		    edgep = edgep->next;
	        }
	    }

            /*
             * The abort_render flag maybe set in render_dmap_line.
             */
            if (abort_render) {
                y_bucket[y] = active_list;  /* Save for later cleanup */
                return;
            }

            y += reverse_scan ? 1 : -1;
	}
    }
}



/*
 * Render depthmaps for the lightsources that will cast
 * shadows. Place the camera in the position of each lightsource
 * in turn and render the depthmap. Store the matrix converting
 * from world coordinates to depthmap coordinates together with 
 * the depthmap.
 */
void
shadowmaps_create(size)
    int size;
{
    Transf_mat      view_mat;
    Vector          view_vec;
    Vector          tmpv;
    double          tmp;
    Lightsource    *lp;
    Camera         *tmp_camera;
    Camera         *light_camera;
    bool            backface_tmp;

    depthmap_size = size;
    depthmaps_create();

    y_bucket = (Edge **)scalloc(depthmap_size, sizeof(Edge *));

    tmp_camera = sipp_current_camera;
    light_camera = camera_create();
    *light_camera = *sipp_current_camera;
    backface_tmp = show_backfaces;
    show_backfaces = TRUE;

    for (lp = lightsrc_stack; lp != NULL && !abort_render; lp = lp->next) {
        if (lp->shadow.active) {
            VecCopy(light_camera->position, 
                    ((Spot_light_info *)(lp->info))->pos);
            VecCopy(light_camera->lookat, 
                    ((Spot_light_info *)(lp->info))->point);
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
            tmp = tmpv.x; tmpv.x = tmpv.y; tmpv.y = tmpv.z; tmpv.z = tmp;
            if (fabs(tmpv.x - tmpv.y) < 1e-5
                && fabs(tmpv.x - tmpv.z) < 1e-5
                && fabs(tmpv.z - tmpv.y) < 1e-5) {
                
                tmpv.x = -tmpv.x;
            }
            VecCross(light_camera->up, tmpv, view_vec);

            lp->shadow.bias = VecLen(view_vec) * 0.005;
            get_view_transf(&view_mat, light_camera, PHONG);
            lp->shadow.matrix = view_mat;

            MatCopy(&curr_mat, &ident_matrix);

            traverse_object_tree(sipp_world, &view_mat, 
                                 depthmap_size - 1, depthmap_size - 1, 
                                 PHONG);
            
            scan_depthmap(lp->shadow.d_map);
        }
    }

    camera_use(tmp_camera);
    camera_destruct(light_camera);
    show_backfaces = backface_tmp;

    if (abort_render) {
        clean_up_y_bucket(depthmap_size);
    }

    sfree(y_bucket);
}
    


/*
 * Destoy all the shadowmaps created by shadowmaps_create()
 */
void
shadowmaps_destruct()
{
    depthmaps_destruct();
}



/*
 * "Main" functions in rendering. Allocate y-bucket, transform vertices
 * into viewing coordinates, make edges and sort them into the y-bucket.
 * Call scan_and_render to do the real work.
 */
static void
render_main(xres, yres, storage_mode, render_mode, oversampling, field)
    int   xres, yres;
    int   storage_mode;
    int   render_mode;
    int   oversampling;
    int   field;
{
    Transf_mat      view_mat;
    int i;

    get_view_transf(&view_mat, sipp_current_camera, render_mode);

    abort_render = FALSE;
    switch (render_mode) {
      case LINE:
        if (storage_mode == PBM_FILE) {
            im_data = sipp_bitmap_create(xres, yres);
            pixel_set = sipp_bitmap_line;
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
    traverse_object_tree(sipp_world, &view_mat, xres - 1, yres - 1, 
                         render_mode);

    switch (render_mode) {
      case LINE:
        if (storage_mode == PBM_FILE) {
            sipp_bitmap_write(image_file, im_data);
            sipp_bitmap_destruct(im_data);
        }
        break;

      case FLAT:
      case GOURAUD:
      case PHONG:
        if (abort_render) {
            clean_up_y_bucket (yres);
            sfree(y_bucket);
            break;
        }
        scan_and_render(xres, yres, storage_mode, render_mode, 
                        oversampling, field);
        if (abort_render) {
            clean_up_y_bucket(yres);
        }
        sfree(y_bucket);
        if (auto_shadows) {
            shadowmaps_destruct();
        }
        break;
    }
}
    

void
render_image_file(xres, yres, im_file, render_mode, oversampling)
    int   xres, yres;
    FILE *im_file;
    int   render_mode;
    int   oversampling;
{
    image_file = im_file;

    if (render_mode == LINE) {
        render_main(xres, yres, PBM_FILE, render_mode, oversampling, BOTH);
    } else {
        render_main(xres, yres, PPM_FILE, render_mode, oversampling, BOTH);
    }
}


void
render_image_func(xres, yres, pixel_func, data, render_mode, oversampling)
    int     xres, yres;
    void  (*pixel_func)();
    void   *data;
    int     render_mode;
    int     oversampling;
{
    im_data = data;
    pixel_set = pixel_func;
    render_main(xres, yres, FUNCTION, render_mode, oversampling, BOTH);
}


void
render_field_file(xres, yres, im_file, render_mode, oversampling, field)
    int   xres, yres;
    FILE *im_file;
    int   render_mode;
    int   oversampling;
    int   field;
{
    image_file = im_file;

    if (render_mode == LINE) {
        fprintf(stderr, "render_field_file: Can't render line fields\n");
        return;
    } else {
        render_main(xres, yres, PPM_FILE, render_mode, oversampling, field);
    }
}


void
render_field_func(xres, yres, pixel_func, data, render_mode, 
                  oversampling, field)
    int     xres, yres;
    void  (*pixel_func)();
    void   *data;
    int     render_mode;
    int     oversampling;
    int     field;
{
    if (render_mode == LINE) {
        fprintf(stderr, "render_field_func: Can't render line fields\n");
        return;
    }

    im_data = data;
    pixel_set = pixel_func;
    render_main(xres, yres, FUNCTION, render_mode, oversampling, field);
}




/*============= Functions that handles global initializations==============*/


/*
 * Function to terminate rendering prematurely.
 */
void
sipp_render_terminate()
{
    abort_render = TRUE;
}


/*
 * If the argument is TRUE, render the scanlines in reverse.
 */
void
sipp_render_direction(direction)
    bool direction;
{
    reverse_scan = direction;
}


/*
 * Save a function pointer to call during the rendering process.  This function
 * is used to handle X-windows updates, etc.  Period is the frequence of that
 * the update is called.  The unit of period is every pixel rendered and units
 * of work that take a similar amount of time.  This is very approimate.
 * Call with proc NULL to disable updates.
 */
void
sipp_set_update_callback(func, client_data, period)
    Update_func *func;
    void        *client_data;
    int          period;
{
    update_func = func;
    update_client_data = client_data;
    update_period = period;
}



/*
 * If called with TRUE as argument, no backface culling will 
 * be performed. If a polygon is backfacing it will be rendered
 * as facing in the opposit direction.
 */
void
sipp_show_backfaces(flag)
    bool flag;
{
    show_backfaces = flag;
}



/*
 * If called with TRUE, objects will cast shadows. The second
 * argument is then used as the size of the depthmaps.
 */
void
sipp_shadows(flag, size)
    bool flag;
    int  size;
{
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
void
sipp_background(red, grn, blu)
    double red, grn, blu;
{
    sipp_bgcol.red = red;
    sipp_bgcol.grn = grn;
    sipp_bgcol.blu = blu;
}



/*
 * Necessary initializations.
 */
void
sipp_init()
{
    objects_init();
    lightsource_init();
    camera_init();
    sipp_shadows(FALSE, 0);
    sipp_show_backfaces(FALSE);
    sipp_render_direction(TOP_TO_BOTTOM);
    sipp_background(0.0, 0.0, 0.0);
    sipp_set_update_callback(NULL, NULL, 0);
}
