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
 ** mask.c - Mask shader: use a masking function to select between 
 **          two shaders.
 **/

#include <sipp.h>
#include <shaders.h>

void
mask_shader(pos, normal, texture, view_vec, lights, md, color, opacity)
    Vector      *pos;
    Vector      *normal;
    Vector      *texture;
    Vector      *view_vec;
    Lightsource *lights;
    Mask_desc   *md;
    Color       *color;
    Color       *opacity;
{
    if (md->masker(md->mask_data, texture)) {
        md->t_shader(pos, normal, texture, view_vec, lights, md->t_surface, 
                     color, opacity); 
    } else {
        md->f_shader(pos, normal, texture, view_vec, lights, md->f_surface, 
                     color, opacity); 
    }
}
