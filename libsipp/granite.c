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
 ** granite.c - Granite shader: using a 1/f fractal noise function for
 **             mixing color values.
 **/

#include <math.h>
#include <stdio.h>

#include <sipp.h>

#include <geometric.h>
#include <noise.h>
#include <shaders.h>

/*
 * WIDTH is the size of the area the sample covers, in the units of P;
 * the octaves of noise finer than that are left out (see noise.h).
 */
static void granite(Vector *p, double width, Color *color, Granite_desc *gd) {
  int i;
  Vector v;
  double temp, n = 0.5, freq = 1.0;
  double cover = width, weight;

  v = *p;
  for (i = 0; i < 6; freq *= 2.0, i++) {
    v.x *= 4.0 * freq;
    v.y *= 4.0 * freq;
    v.z *= 4.0 * freq;
    cover *= 4.0 * freq;
    weight = noise_weight(cover);
    if (weight <= 0.0) {
      break;
    }
    temp = 0.5 * noise(&v);
    /*        temp = fabs(temp);*/
    n += weight * temp / freq;
  }

  color->red = gd->col1.red * n + gd->col2.red * (1.0 - n);
  color->grn = gd->col1.grn * n + gd->col2.grn * (1.0 - n);
  color->blu = gd->col1.blu * n + gd->col2.blu * (1.0 - n);
}

void granite_shader(const Shade_point *sp, Lightsource *lights, void *gd_,
                    Color *color, Color *opacity) {
  Granite_desc *gd = (Granite_desc *)gd_;
  Vector taps[4];
  Vector tmp;
  Color c;
  Surf_desc surface;
  double width;
  int ntaps, i;

  noise_init();

  /* See marble_shader() on the taps. */
  ntaps = shade_texture_taps(sp, 4, taps, &width);
  surface.color.red = surface.color.grn = surface.color.blu = 0.0;
  for (i = 0; i < ntaps; i++) {
    VecScalMul(tmp, gd->scale, taps[i]);
    granite(&tmp, width * gd->scale, &c, gd);
    surface.color.red += c.red / ntaps;
    surface.color.grn += c.grn / ntaps;
    surface.color.blu += c.blu / ntaps;
  }
  surface.ambient = gd->ambient;
  surface.specular = gd->specular;
  surface.c3 = gd->c3;
  surface.opacity = gd->opacity;
  basic_shader(sp, lights, &surface, color, opacity);
}
