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

#include <shaders.h>
#include <sipp.h>

void mask_shader(const Shade_point *sp, Lightsource *lights, void *md_,
                 Color *color, Color *opacity) {
  Mask_desc *md = (Mask_desc *)md_;
  Color f_color, f_opacity;
  double t;

  t = md->masker(md->mask_data, sp);
  if (t >= 1.0) {
    md->t_shader(sp, lights, md->t_surface, color, opacity);
  } else if (t <= 0.0) {
    md->f_shader(sp, lights, md->f_surface, color, opacity);
  } else {
    md->t_shader(sp, lights, md->t_surface, color, opacity);
    md->f_shader(sp, lights, md->f_surface, &f_color, &f_opacity);
    color->red = f_color.red + t * (color->red - f_color.red);
    color->grn = f_color.grn + t * (color->grn - f_color.grn);
    color->blu = f_color.blu + t * (color->blu - f_color.blu);
    opacity->red = f_opacity.red + t * (opacity->red - f_opacity.red);
    opacity->grn = f_opacity.grn + t * (opacity->grn - f_opacity.grn);
    opacity->blu = f_opacity.blu + t * (opacity->blu - f_opacity.blu);
  }
}
