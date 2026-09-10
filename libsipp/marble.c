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
 ** marble.c - Marble shader: simulates marble using noise & turbulence
 **/

#include <math.h>
#include <stdio.h>

#include <sipp.h>

#include <geometric.h>
#include <noise.h>
#include <shaders.h>

/*
 * WIDTH is the size of the area the sample covers, in the units of P;
 * turbulence finer than that is left out (see noise.h).
 */
static void marble(Vector *p, double width, Color *color, Marble_desc *md) {
  double x, t;

  x = p->x + turbulence_filtered(p, 7, width) * 5;
  x = sin(x);
  if (x > -0.1 && x < 0.1) {
    color->red = md->strip.red;
    color->grn = md->strip.grn;
    color->blu = md->strip.blu;
  } else if (x > -0.9 && x < 0.9) {
    /* You are not supposed to understand this... */
    t = 7.0 / 6.0 - 1.0 / (6.25 * fabs(x) + 0.375);
    color->red = md->strip.red + t * (md->base.red - md->strip.red);
    color->grn = md->strip.grn + t * (md->base.grn - md->strip.grn);
    color->blu = md->strip.blu + t * (md->base.blu - md->strip.blu);
  } else {
    color->red = md->base.red;
    color->grn = md->base.grn;
    color->blu = md->base.blu;
  }
}

void marble_shader(const Shade_point *sp, Lightsource *lights, void *md_,
                   Color *color, Color *opacity) {
  Marble_desc *md = (Marble_desc *)md_;
  Vector taps[4];
  Vector tmp;
  Color c;
  Surf_desc surface;
  double width;
  int ntaps, i;

  noise_init();

  /*
   * Average the texture over a few points along the long axis of the
   * sample, each filtered to its short axis (anisotropic filtering).
   */
  ntaps = shade_texture_taps(sp, 4, taps, &width);
  surface.color.red = surface.color.grn = surface.color.blu = 0.0;
  for (i = 0; i < ntaps; i++) {
    VecScalMul(tmp, md->scale, taps[i]);
    marble(&tmp, width * md->scale, &c, md);
    surface.color.red += c.red / ntaps;
    surface.color.grn += c.grn / ntaps;
    surface.color.blu += c.blu / ntaps;
  }
  surface.ambient = md->ambient;
  surface.specular = md->specular;
  surface.c3 = md->c3;
  surface.opacity = md->opacity;
  basic_shader(sp, lights, &surface, color, opacity);
}
