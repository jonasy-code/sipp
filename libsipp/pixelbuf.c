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
 ** pixel.c - Functions that handle the pixel buffer
 **/


#include <string.h>

#include <sipp.h>
#include <smalloc.h>
#include <rendering.h>
#include <lightsource.h>
#include <viewpoint.h>
#include <pixelbuf.h>


/*
 * Background color.
 */
Color sipp_bgcol;


/*
 * Prototypes of internal functions.
 */
static int
pixel_alloc(Pixel_buffer *pb);


/*
 * Initialize the pixel buffer.
 */
void
pixels_setup(Pixel_buffer *pb, int init_size)
{
    pb->pixbuf = (Pixel_info *)scalloc(init_size, sizeof(Pixel_info));
    pb->pixbuf_size = init_size;
    pb->size_delta = init_size / 2;
    pb->first_free = 0;
    pb->shade_cache = NULL;
    pb->shade_count = NULL;
    pb->shade_npixels = 0;
}


/*
 * Free memory used by pixel_buffer.
 */
void
pixels_free(Pixel_buffer *pb)
{
    if (pb->pixbuf != NULL) {
        sfree(pb->pixbuf);
    }
    pb->pixbuf = NULL;
    shade_cache_free(pb);
}


/*
 * Renitialize the free_list.
 */
void
pixels_reinit(Pixel_buffer *pb)
{
    pb->first_free = 0;
}
    

/*
 * Allocate a Pixel_info struct from the free list.
 * Realloc a larger pixbuf if needed.
 */
static int
pixel_alloc(Pixel_buffer *pb)
{
    if (pb->first_free == pb->pixbuf_size) {
        pb->pixbuf_size += pb->size_delta;
        pb->pixbuf = (Pixel_info *)srealloc(pb->pixbuf, 
                                            pb->pixbuf_size 
                                            * sizeof(Pixel_info));
    }

    pb->pixbuf[pb->first_free].next = -1;
    pb->first_free++;
    return (pb->first_free - 1);
}
    

/*
 * Create a new Pixel_info struct containing DEPTH, COLOR and OPACITY and
 * insert it into PIXEL.
 */
int
pixel_insert(Pixel_buffer *pb, int pixel, Vector *worldstep, Vector *texturestep, Vector *normalstep, double depth, double hden, double offset, Active_edge *edge)
{
    Pixel_info *pixbuf;
    int  pixref1;
    int  pixref2;
    int  tmp;

    tmp = pixel_alloc(pb);
    pixbuf = pb->pixbuf;               /* pixel_alloc() may have moved it */
    pixbuf[tmp].worldstep = *worldstep;
    pixbuf[tmp].texturestep = *texturestep;
    pixbuf[tmp].normalstep = *normalstep;
    pixbuf[tmp].offset = offset;
    pixbuf[tmp].depth = depth;
    pixbuf[tmp].edge = edge;
    pixbuf[tmp].hden = hden;

    if (pixel == -1) {
        return tmp;
    } else if (depth < pixbuf[pixel].depth) {
        pixbuf[tmp].next = pixel;
        return tmp;
    } else {
        pixref1 = pixel;
        pixref2 = pixbuf[pixel].next;
        while (pixref2 != -1 && pixbuf[pixref2].depth < depth) {
            pixref1 = pixref2;
            pixref2 = pixbuf[pixref2].next;
        } 
        pixbuf[pixref1].next = tmp;
        pixbuf[tmp].next = pixref2;
        return pixel;
    }
}


/*
 * Sum the resulting color in a pixel and store it in the Color
 * struct pointed to by RESULT.
 * If there are no visible polygons in this pixel,  return the
 * background color.
 */
void
shade_cache_setup(Pixel_buffer *pb, int npixels)
{
    pb->shade_cache = (Shade_entry *)smalloc(npixels * SHADE_SLOTS 
                                             * sizeof(Shade_entry));
    pb->shade_count = (int *)scalloc(npixels, sizeof(int));
    pb->shade_npixels = npixels;
}


void
shade_cache_clear(Pixel_buffer *pb)
{
    if (pb->shade_count != NULL) {
        memset(pb->shade_count, 0, pb->shade_npixels * sizeof(int));
    }
}


void
shade_cache_free(Pixel_buffer *pb)
{
    if (pb->shade_cache != NULL) {
        sfree(pb->shade_cache);
        sfree(pb->shade_count);
    }
    pb->shade_cache = NULL;
    pb->shade_count = NULL;
    pb->shade_npixels = 0;
}


/*
 * Look up a cached shader result for polygon POLYGON in output pixel
 * SLOT.  Returns TRUE and fills in COLOR/OPACITY if found.
 */
static bool
shade_cache_lookup(Pixel_buffer *pb, int slot, int polygon, Color *color, Color *opacity)
{
    Shade_entry *e;
    int          i;

    e = pb->shade_cache + slot * SHADE_SLOTS;
    for (i = 0; i < pb->shade_count[slot]; i++) {
        if (e[i].polygon == polygon) {
            *color = e[i].color;
            *opacity = e[i].opacity;
            return TRUE;
        }
    }
    return FALSE;
}


static void
shade_cache_insert(Pixel_buffer *pb, int slot, int polygon, Color *color, Color *opacity)
{
    Shade_entry *e;

    if (pb->shade_count[slot] < SHADE_SLOTS) {
        e = pb->shade_cache + slot * SHADE_SLOTS + pb->shade_count[slot]++;
        e->polygon = polygon;
        e->color = *color;
        e->opacity = *opacity;
    }
}


/*
 * Walk the fragments of PIXEL front to back, shading and compositing
 * them into RESULT.
 *
 * CACHE_SLOT is the index of the output pixel this sub-sample belongs
 * to, or -1.  When it is >= 0, shader results are looked up in and
 * stored into the shading cache, so that each polygon is shaded once
 * per output pixel and the result reused for all of the pixel's
 * sub-samples.  Depth sorting and transparency are still resolved per
 * sub-sample; only the shader call is shared.
 */
void
pixel_collect(Pixel_buffer *pb, int pixel, Color *result, int render_mode, int cache_slot)
{
    Pixel_info *pixbuf = pb->pixbuf;
    Color    frac;
    Color    opacity_sum;
    Color    surf_color;
    Color    surf_opacity;
    Active_edge *edge;
    Vector   world;
    Vector   texture;
    Vector   normal;
    Vector   viewer;
    int      pixref;
    bool     pixel_full;

    result->red = result->grn = result->blu = 0.0;
    opacity_sum.red = opacity_sum.grn = opacity_sum.blu = 0.0;

    pixref = pixel;
    pixel_full = FALSE;

    while (pixref != -1) {
        /*
         * Do the interpolations approptiate for the various
         * rendering modes and call the shader if it is needed.
         */
        edge = pixbuf[pixref].edge;
        switch (render_mode) {

          /*
           * In PHONG mode we interpolate to get the new world, texture,
           * and normal and call the shader.
           */
          case PHONG:
            VecAddS(world, pixbuf[pixref].offset, 
                    pixbuf[pixref].worldstep, edge->world);
            VecScalMul(world, 1.0 / pixbuf[pixref].hden, world);
            VecAddS(texture, pixbuf[pixref].offset, 
                    pixbuf[pixref].texturestep, edge->texture);
            VecScalMul(texture, 1.0 / pixbuf[pixref].hden, texture);
            VecAddS(normal, pixbuf[pixref].offset, 
                    pixbuf[pixref].normalstep, edge->normal);
            if (cache_slot >= 0 
                && shade_cache_lookup(pb, cache_slot, edge->edge->polygon, 
                                      &surf_color, &surf_opacity)) {
                break;
            }
            VecSub(viewer, sipp_current_camera->position, world);
            vecnorm(&viewer);
            edge->edge->surface->shader(&world, &normal, &texture, 
                                        &viewer, lightsrc_stack, 
                                        edge->edge->surface->surface, 
                                  &surf_color, &surf_opacity);
            if (cache_slot >= 0) {
                shade_cache_insert(pb, cache_slot, edge->edge->polygon, 
                                   &surf_color, &surf_opacity);
            }
            break;

          /*
           * GOURAUD mode have color and opacity stored in
           * the normal and texture vectors (ugly...) but they need to
           * be interpolated across the surface.
           */
          case GOURAUD:
            VecAddS(texture, pixbuf[pixref].offset, 
                    pixbuf[pixref].texturestep, edge->texture);
            VecScalMul(texture, 1.0 / pixbuf[pixref].hden, texture);
            VecAddS(normal, pixbuf[pixref].offset, 
                    pixbuf[pixref].normalstep, edge->normal);
            VecScalMul(normal, 1.0 / pixbuf[pixref].hden, normal);
            surf_color.red = normal.x;
            surf_color.grn = normal.y;
            surf_color.blu = normal.z;
            surf_opacity.red = texture.x;
            surf_opacity.grn = texture.y;
            surf_opacity.blu = texture.z;
            break;

          /*
           * FLAT mode have color and opacity stored in
           * the normal and texture vectors (ugly...).
           * No interpolated needed.
           */
          case FLAT:
            surf_color.red = edge->normal.x;
            surf_color.grn = edge->normal.y;
            surf_color.blu = edge->normal.z;
            surf_opacity.red = edge->texture.x;
            surf_opacity.grn = edge->texture.y;
            surf_opacity.blu = edge->texture.z;
            break;
        }

        frac.red = surf_opacity.red;
        if (frac.red + opacity_sum.red > 1.0) {
            frac.red = 1.0 - opacity_sum.red;
        }
        frac.grn = surf_opacity.grn;
        if (frac.grn + opacity_sum.grn > 1.0) {
            frac.grn = 1.0 - opacity_sum.grn;
        }
        frac.blu = surf_opacity.blu;
        if (frac.blu + opacity_sum.blu > 1.0) {
            frac.blu = 1.0 - opacity_sum.blu;
        }

        result->red += frac.red * surf_color.red;
        result->grn += frac.grn * surf_color.grn;
        result->blu += frac.blu * surf_color.blu;

        opacity_sum.red += frac.red;
        opacity_sum.grn += frac.grn;
        opacity_sum.blu += frac.blu;

        if (opacity_sum.red >= 1.0 && opacity_sum.grn >= 1.0 
            && opacity_sum.blu >= 1.0) {
            pixel_full = TRUE;
            break;
        }
        pixref = pixbuf[pixref].next;
    }

    if (!pixel_full) {
        result->red += ((opacity_sum.red >= 1.0) 
                        ? 0.0 : 1.0 - opacity_sum.red) * sipp_bgcol.red; 
        result->grn += ((opacity_sum.grn >= 1.0) 
                        ? 0.0 : 1.0 - opacity_sum.grn) * sipp_bgcol.grn; 
        result->blu += ((opacity_sum.blu >= 1.0) 
                        ? 0.0 : 1.0 - opacity_sum.blu) * sipp_bgcol.blu; 
    }
}
