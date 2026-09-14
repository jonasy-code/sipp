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
 ** basic_shader.c - Basic shading model, somewhat modified and
 **                  simplified version of Blinn's
 **/

#include <math.h>

#include <sipp.h>

#include <geometric.h>

double shade_texture_width(const Shade_point *sp) {
  double dx = VecLen(sp->dtdx);
  double dy = VecLen(sp->dtdy);

  return (dx > dy) ? dx : dy;
}

double shade_world_width(const Shade_point *sp) {
  double dx = VecLen(sp->dpdx);
  double dy = VecLen(sp->dpdy);

  return (dx > dy) ? dx : dy;
}

/*
 * The half axes of the footprint ellipse are the singular values of
 * the 3 x 2 matrix [dtdx dtdy]: the square roots of the eigenvalues of
 * its 2 x 2 Gram matrix.  The major axis is the image of the
 * eigenvector of the larger eigenvalue.
 */
void shade_texture_footprint(const Shade_point *sp, Vector *major,
                             double *minor) {
  double a = VecDot(sp->dtdx, sp->dtdx);
  double b = VecDot(sp->dtdx, sp->dtdy);
  double c = VecDot(sp->dtdy, sp->dtdy);
  double disc = sqrt((a - c) * (a - c) + 4.0 * b * b);
  double l1 = 0.5 * (a + c + disc);
  double l2 = 0.5 * (a + c - disc);
  double e0, e1, len;

  *minor = sqrt(l2 > 0.0 ? l2 : 0.0);
  if (l1 <= 0.0) {
    MakeVector(*major, 0.0, 0.0, 0.0);
    return;
  }
  if (fabs(l1 - c) > fabs(l1 - a)) {
    e0 = l1 - c;
    e1 = b;
  } else {
    e0 = b;
    e1 = l1 - a;
  }
  if (e0 == 0.0 && e1 == 0.0) { /* Round footprint: any direction */
    e0 = 1.0;
  }
  VecComb(*major, e0, sp->dtdx, e1, sp->dtdy);
  len = VecLen(*major);
  VecScalMul(*major, (len > 0.0) ? sqrt(l1) / len : 0.0, *major);
}

int shade_texture_taps(const Shade_point *sp, int max_taps, Vector *points,
                       double *width) {
  Vector major;
  double minor, len, t;
  int n, i;

  shade_texture_footprint(sp, &major, &minor);
  len = VecLen(major);
  if (len <= 0.0 || max_taps < 2) {
    points[0] = sp->texture;
    *width = len;
    return 1;
  }
  n = (minor > 0.0) ? (int)ceil(len / minor) : max_taps;
  if (n > max_taps) {
    n = max_taps;
  }
  if (n < 1) {
    n = 1;
  }
  *width = len / n;
  if (*width < minor) {
    *width = minor;
  }
  for (i = 0; i < n; i++) {
    t = (i + 0.5) / n - 0.5;
    VecAddS(points[i], t, major, sp->texture);
  }
  return n;
}

void basic_shader(const Shade_point *sp, Lightsource *lights, void *sd_,
                  Color *color, Color *opacity) {
  Surf_desc *sd = (Surf_desc *)sd_;
  Vector unit_norm;
  Vector light_dir;
  Vector specular;
  Color diffsum;
  Color specsum;
  double coss, cosi;
  double c3two;
  double shadow_fact;
  double tmp;
  Lightsource *lp;

  VecCopy(unit_norm, sp->normal);
  vecnorm(&unit_norm);

  diffsum.red = diffsum.grn = diffsum.blu = 0.0;
  specsum.red = specsum.grn = specsum.blu = 0.0;
  c3two = sd->c3 * sd->c3;

  for (lp = lights; lp != (Lightsource *)0; lp = lp->next) {

    shadow_fact = light_eval(lp, &sp->pos, &light_dir);

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
      coss = VecDot(specular, sp->view_vec);
      if (coss > 0) {
        tmp = (shadow_fact * c3two * coss / (coss * coss * (c3two - 1) + 1));
        specsum.red += lp->color.red * tmp;
        specsum.grn += lp->color.grn * tmp;
        specsum.blu += lp->color.blu * tmp;
      }
    }
  }

  if (diffsum.red > 1.0)
    diffsum.red = 1.0;
  if (diffsum.grn > 1.0)
    diffsum.grn = 1.0;
  if (diffsum.blu > 1.0)
    diffsum.blu = 1.0;
  if (specsum.red > 1.0)
    specsum.red = 1.0;
  if (specsum.grn > 1.0)
    specsum.grn = 1.0;
  if (specsum.blu > 1.0)
    specsum.blu = 1.0;

  color->red = (sd->color.red * (sd->ambient + diffsum.red) +
                sd->specular * specsum.red);
  if (color->red > 1.0)
    color->red = 1.0;
  color->grn = (sd->color.grn * (sd->ambient + diffsum.grn) +
                sd->specular * specsum.grn);
  if (color->grn > 1.0)
    color->grn = 1.0;
  color->blu = (sd->color.blu * (sd->ambient + diffsum.blu) +
                sd->specular * specsum.blu);
  if (color->blu > 1.0)
    color->blu = 1.0;

  opacity->red = sd->opacity.red;
  opacity->grn = sd->opacity.grn;
  opacity->blu = sd->opacity.blu;
}
