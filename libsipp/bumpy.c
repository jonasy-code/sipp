/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright 1992 Jonas Yngvesson, Inge Wallin
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
 ** bumpy.c - Bumpy shader: simulates an bumpy surfaces using noise and Dnoise
 **/

#include <math.h>
#include <stdio.h>

#include <sipp.h>

#include <geometric.h>
#include <noise.h>
#include <shaders.h>

void bumpy_shader(const Shade_point *sp, Lightsource *lights, void *bd_,
                  Color *color, Color *opacity) {
  Bumpy_desc *bd = (Bumpy_desc *)bd_;
  Shade_point bumped = *sp; /* The point with its normal disturbed */
  Vector tmp;
  double no;
  double weight;

  noise_init();

  vecnorm(&bumped.normal);
  VecScalMul(tmp, bd->scale, sp->texture);

  /*
   * Bumps finer than the sample are smoothed away, so that they do
   * not alias into a sparkling surface at a distance.
   */
  weight = noise_weight(shade_texture_width(sp) * bd->scale);
  if (weight > 0.0 && ((bd->bumpflag && bd->holeflag) ||
                       ((no = noise(&tmp)) < 0.0 && bd->bumpflag) ||
                       (no > 0.0 && bd->holeflag))) {
    tmp = Dnoise(&tmp);
    VecAddS(bumped.normal, weight, tmp, bumped.normal);
  }

  bd->shader(&bumped, lights, bd->surface, color, opacity);
}
