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
 ** basic_shader.c - Basic shading model, somewhat modified and 
 **                  simplified version of Blinn's
 **/

#include <math.h>

#include <sipp.h>
#include <geometric.h>

void
basic_shader(pos, normal, texture, view_vec, lights, sd, color, opacity)
    Vector      *pos;
    Vector      *normal;
    Vector      *texture;
    Vector      *view_vec;
    Lightsource *lights;
    Surf_desc   *sd;
    Color       *color;
    Color       *opacity;
{
    Vector       unit_norm;
    Vector       light_dir;
    Vector       specular;
    Color        diffsum;
    Color        specsum;
    double       coss, cosi;
    double       c3two;
    double       shadow_fact;
    double       tmp;
    Lightsource *lp;

    VecCopy(unit_norm, *normal);
    vecnorm(&unit_norm);
    diffsum.red = diffsum.grn = diffsum.blu = 0.0;
    specsum.red = specsum.grn = specsum.blu = 0.0;
    c3two = sd->c3 * sd->c3;

    for (lp = lights; lp != (Lightsource *)0; lp = lp->next) {

        shadow_fact = light_eval(lp, pos, &light_dir);

        if (shadow_fact > 0.0001) {
            cosi = VecDot(light_dir, unit_norm);
            if (cosi > 0) {
                tmp = cosi * shadow_fact;
                diffsum.red += lp->color.red * tmp;
                diffsum.grn += lp->color.grn * tmp;
                diffsum.blu += lp->color.blu * tmp;
            }
        
            cosi *= 2.0;
            VecComb(specular, -1.0, light_dir, cosi, unit_norm);
            coss = VecDot(specular, *view_vec);
            if (coss > 0) {
                tmp = (shadow_fact * c3two * coss 
                       / (coss * coss * (c3two -1) + 1));
                specsum.red += lp->color.red * tmp;
                specsum.grn += lp->color.grn * tmp;
                specsum.blu += lp->color.blu * tmp;
            }
        }
    }

    if (diffsum.red > 1.0) diffsum.red = 1.0;
    if (diffsum.grn > 1.0) diffsum.grn = 1.0;
    if (diffsum.blu > 1.0) diffsum.blu = 1.0;
    if (specsum.red > 1.0) specsum.red = 1.0;
    if (specsum.grn > 1.0) specsum.grn = 1.0;
    if (specsum.blu > 1.0) specsum.blu = 1.0;

    color->red = (sd->color.red * (sd->ambient + diffsum.red) 
                  + sd->specular * specsum.red); 
    if (color->red > 1.0) color->red = 1.0;

    color->grn = (sd->color.grn * (sd->ambient + diffsum.grn) 
                  + sd->specular * specsum.grn); 
    if (color->grn > 1.0) color->grn = 1.0;

    color->blu = (sd->color.blu * (sd->ambient + diffsum.blu) 
                  + sd->specular * specsum.blu);
    if (color->blu > 1.0) color->blu = 1.0;

    opacity->red = sd->opacity.red;
    opacity->grn = sd->opacity.grn;
    opacity->blu = sd->opacity.blu;
}



