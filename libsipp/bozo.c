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
 ** bozo.c - Bozo shader: color objects a la Bozo the clown.
 **/

#include <math.h>
#include <stdio.h>

#include <sipp.h>
#include <noise.h>
#include <shaders.h>


extern bool noise_ready;

void
bozo_shader(pos, normal, texture, view_vec, lights, bd, color, opacity)
    Vector      *pos;
    Vector      *normal;
    Vector      *texture;
    Vector      *view_vec;
    Lightsource *lights;
    Bozo_desc   *bd;
    Color       *color;
    Color       *opacity;
{
    Vector     tmp;
    Surf_desc  surface;
    double     noiseval;
    int        i;

    if (!noise_ready) {
        noise_init();
    }

    VecScalMul(tmp, bd->scale, *texture);
    noiseval = noise(&tmp);

    i = (noiseval + 1) * bd->no_of_cols / 2.0;
    surface.color    = bd->colors[i];
    surface.ambient  = bd->ambient;
    surface.specular = bd->specular;
    surface.c3       = bd->c3;
    surface.opacity  = bd->opacity;
    basic_shader(pos, normal, texture, view_vec, lights, &surface, 
                 color, opacity);
}
