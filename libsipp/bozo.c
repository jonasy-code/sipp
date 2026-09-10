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

void bozo_shader(const Shade_point *sp, Lightsource *lights, void *bd_,
                 Color *color, Color *opacity) {
  Bozo_desc *bd = (Bozo_desc *)bd_;
  Vector taps[4];
  Vector tmp;
  Surf_desc surface;
  double noiseval, width;
  int ntaps, k, i;

  noise_init();

  /* See marble_shader() on the taps. */
  ntaps = shade_texture_taps(sp, 4, taps, &width);
  noiseval = 0.0;
  for (k = 0; k < ntaps; k++) {
    VecScalMul(tmp, bd->scale, taps[k]);
    noiseval += noise_filtered(&tmp, width * bd->scale) / ntaps;
  }
  i = (noiseval + 1) * bd->no_of_cols / 2.0;
  surface.color = bd->colors[i];
  surface.ambient = bd->ambient;
  surface.specular = bd->specular;
  surface.c3 = bd->c3;
  surface.opacity = bd->opacity;
  basic_shader(sp, lights, &surface, color, opacity);
}
