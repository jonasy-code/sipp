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
 ** phong_shader.c - The traditional Phong shading function.
 **/

#include <math.h>

#include <sipp.h>
#include <geometric.h>
#include <shaders.h>


void
phong_shader(pos, normal, texture, view_vec, lights, pd, color, opacity)
    Vector      *pos;
    Vector      *normal;
    Vector      *texture;
    Vector      *view_vec;
    Lightsource *lights;
    Phong_desc  *pd;
    Color       *color;
    Color       *opacity;
{
    Vector       unit_norm;
    Vector       light_dir;
    Vector       specular;
    Color        diffsum;
    Color        specsum;
    double       cos_theta;
    double       cos_alpha;
    double       light_fraction;
    double       spec_factor;
    Lightsource *lp;

    VecCopy(unit_norm, *normal);
    vecnorm(&unit_norm);
    diffsum.red = diffsum.grn = diffsum.blu = 0.0;
    specsum.red = specsum.grn = specsum.blu = 0.0;

    for (lp = lights; lp != (Lightsource *)0; lp = lp->next) {

        light_fraction = light_eval(lp, pos, &light_dir);

        if (light_fraction > 0.0001) {
            cos_theta = VecDot(light_dir, unit_norm);
            if (cos_theta > 0) {
                diffsum.red += lp->color.red * cos_theta * light_fraction;
                diffsum.grn += lp->color.grn * cos_theta * light_fraction;
                diffsum.blu += lp->color.blu * cos_theta * light_fraction;
            }
        
            cos_theta *= 2.0;
            VecComb(specular, -1.0, light_dir, cos_theta, unit_norm);
            cos_alpha = VecDot(specular, *view_vec);
            if (cos_alpha > 0) {
                spec_factor = light_fraction * exp(pd->spec_exp
                                                    * log(cos_alpha));  
                specsum.red += lp->color.red * spec_factor;
                specsum.grn += lp->color.grn * spec_factor;
                specsum.blu += lp->color.blu * spec_factor;
            }
        }
    }

    if (diffsum.red > 1.0) diffsum.red = 1.0;
    if (diffsum.grn > 1.0) diffsum.grn = 1.0;
    if (diffsum.blu > 1.0) diffsum.blu = 1.0;
    if (specsum.red > 1.0) specsum.red = 1.0;
    if (specsum.grn > 1.0) specsum.grn = 1.0;
    if (specsum.blu > 1.0) specsum.blu = 1.0;

    color->red = (pd->color.red * (pd->ambient + pd->diffuse * diffsum.red) 
                  + pd->specular * specsum.red); 
    if (color->red > 1.0) color->red = 1.0;

    color->grn = (pd->color.grn * (pd->ambient + pd->diffuse * diffsum.grn) 
                  + pd->specular * specsum.grn); 
    if (color->grn > 1.0) color->grn = 1.0;

    color->blu = (pd->color.blu * (pd->ambient + pd->diffuse * diffsum.blu) 
                  + pd->specular * specsum.blu);
    if (color->blu > 1.0) color->blu = 1.0;

    opacity->red = pd->opacity.red;
    opacity->grn = pd->opacity.grn;
    opacity->blu = pd->opacity.blu;
}



