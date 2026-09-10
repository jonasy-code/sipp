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
 ** wood.c - Wood shader: Simulates wood using multiple skewed "cylinders"
 **          distorted with noise and turbulence.
 **/

#include <math.h>
#include <stdio.h>

#include <sipp.h>

#include <noise.h>
#include <shaders.h>

#define BOARDSIZE 45.0 /* "Log" or "board" size in texture coordinates */

void wood_shader(Vector *pos, Vector *normal, Vector *texture, Vector *view_vec,
                 Lightsource *lights, void *wd_, Color *color, Color *opacity) {
  Wood_desc *wd = (Wood_desc *)wd_;
  Vector tpos;
  Vector tmp;
  Surf_desc surface;
  double chaos;
  double val;
  double val2;
  double skewoff;
  double t;
  double rad;

  noise_init();

  /*
   * Scale the texture coordinates.
   */
  VecScalMul(tpos, wd->scale, *texture);

  /*
   * Get some noise values. Drag out the texture in
   * the direction of the "stem" of the fictive tree, the
   * pattern should vary less along that direction.
   */
  tpos.x *= 0.08;
  chaos = turbulence(&tpos, 5) * 0.5;
  val2 = noise(&tpos);

  /*
   * Make the pattern "semi"-periodic so it looks as if
   * a new board is used at regular intervals
   */
  if (tpos.z > 0.0) {
    tmp.z = floor((tpos.z + BOARDSIZE * 0.5) / BOARDSIZE);
    tpos.z -= tmp.z * BOARDSIZE;
  } else {
    tmp.z = floor((BOARDSIZE * 0.5 - tpos.z) / BOARDSIZE);
    tpos.z += tmp.z * BOARDSIZE;
  }
  if (tpos.y > 0.0) {
    tmp.y = floor((tpos.y + BOARDSIZE * 0.5) / BOARDSIZE);
    tpos.y -= tmp.y * BOARDSIZE;
  } else {
    tmp.y = floor((BOARDSIZE * 0.5 - tpos.y) / BOARDSIZE);
    tpos.y += tmp.y * BOARDSIZE;
  }

  /*
   * Skew the "stem" a bit so the "cylinders" isn't perfectly
   * symmertic about the x-axis. Skew the different "logs"
   * slightly differently.
   */
  tmp.x = 0.0;
  skewoff = noise(&tmp);
  tpos.z -= (0.05 + 0.03 * skewoff) * (texture->x * wd->scale - 2.0);
  tpos.y -= (0.05 + 0.03 * skewoff) * (texture->x * wd->scale - 2.0);

  /*
   * Calculate the distance from the middle of the "stem" and
   * distort this distance with the turbulence value.
   */
  rad = sqrt(tpos.y * tpos.y + tpos.z * tpos.z);
  rad += chaos;
  val = rad - floor(rad);

  /*
   * Choose a color dependent on the distorted distance.
   */
  if (val < 0.1) {
    surface.color.red = wd->base.red;
    surface.color.grn = wd->base.grn;
    surface.color.blu = wd->base.blu;
  } else if (val < 0.9) {
    t = 1.0 - pow(val / 0.8 - 0.1, 6.0);
    surface.color.red = wd->ring.red + t * (wd->base.red - wd->ring.red);
    surface.color.grn = wd->ring.grn + t * (wd->base.grn - wd->ring.grn);
    surface.color.blu = wd->ring.blu + t * (wd->base.blu - wd->ring.blu);
  } else {
    surface.color.red = wd->ring.red;
    surface.color.grn = wd->ring.grn;
    surface.color.blu = wd->ring.blu;
  }

  /*
   * Add a little extra "noise" so the pattern doesn't get
   * too regular, this could be small cracks, or other anomalies
   * in the wood.
   */
  if (val2 < 0.01 && val2 > 0.0) {
    surface.color.red = wd->ring.red;
    surface.color.grn = wd->ring.grn;
    surface.color.blu = wd->ring.blu;
  }

  surface.ambient = wd->ambient;
  surface.specular = wd->specular;
  surface.c3 = wd->c3;
  surface.opacity = wd->opacity;
  basic_shader(pos, normal, texture, view_vec, lights, &surface, color,
               opacity);
}
