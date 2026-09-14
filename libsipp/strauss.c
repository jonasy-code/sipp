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
 ** strauss.c - Shading model designed by Paul S. Strauss
 **             Silicon Graphics Inc.
 **             Described in IEEE CG&A November 1990.
 **
 **             Implemented for SIPP by Jonas Yngvesson
 **/

#include <math.h>

#include <sipp.h>

#include <geometric.h>
#include <shaders.h>

/*
 * Strauss describes, in his article, two functions that simulates
 * fresnel reflection and geometric attenuation. These functions
 * are dependent of an angle between 0 and pi/2 normalized to the
 * range (0.0, 1.0). He also mentions that he uses versions of the
 * functions that depend on the cosine of an angle instead (since
 * that can be calculated with a dot product).
 * These versions are not described in the article so I have empirically
 * tried to recreate them. I don't know if this bears any resemblence
 * to what he used, so any errors are my fault.
 *
 * What i did was only to change x in the functions to (1 - x) and modify
 * the Kf and Kg constants slightly. The original values in the article
 * was: Kf = 1.12, Kg = 1.02
 */

/* Factors used in the approximations of fresnel reflection and of
   geometric attenuation. */
static const double Kf = 1.2;
static const double Kg = 1.031;

/*
 * Function to simulate fresnel reflection.
 */
#define FR(x)                                                                  \
  ((1.0 / ((1.0 - (x) - Kf) * (1.0 - (x) - Kf)) - 1.0 / (Kf * Kf)) /           \
   (1.0 / ((1.0 - Kf) * (1.0 - Kf)) - 1.0 / (Kf * Kf)))

/*
 * Function to simulate geometric attenuation.
 */
#define GA(x)                                                                  \
  ((1.0 / ((1.0 - Kg) * (1.0 - Kg)) -                                          \
    1.0 / ((1.0 - (x) - Kg) * (1.0 - (x) - Kg))) /                             \
   (1.0 / ((1.0 - Kg) * (1.0 - Kg)) - 1.0 / (Kg * Kg)))

void strauss_shader(const Shade_point *sp, Lightsource *lights, void *sd_,
                    Color *color, Color *opacity) {
  Strauss_desc *sd = (Strauss_desc *)sd_;
  Vector unit_normal;  /* Normalized surface normal */
  Vector highlight;    /* Highlight vector */
  double c_alpha;      /* cos(angle between normal and lightvector) */
  double c_beta;       /* cos(angle between highlight & view_vec) */
  double c_gamma;      /* cos(angle between normal & view_vec) */
  double rd;           /* Diffuse reflectivity */
  double rs;           /* Specular reflectivity */
  double rj;           /* Adjusted specular reflectivity */
  double rn;           /* Specular reflectivity at normal incidence */
  double h;            /* Shininess exponent */
  Vector qd;           /* Diffuse reflection factor */
  Vector qs;           /* Specular reflection factor */
  Vector light_dir;    /* Direction to "current" light */
  double light_factor; /* Fraction of light from "current" light */
  Color col;           /* Resulting color */
  Lightsource *lp;

  VecCopy(unit_normal, sp->normal);
  vecnorm(&unit_normal);
  c_gamma = VecDot(unit_normal, sp->view_vec);

  col.red = col.grn = col.blu = 0.0;
  rd = 1.0 - sd->smoothness * sd->smoothness * sd->smoothness;

  for (lp = lights; lp != (Lightsource *)0; lp = lp->next) {
    /*
     * light_factor accounts for shadows and spotlight attenuation
     * (1.0 for an unobstructed directional or point light).
     */
    light_factor = light_eval(lp, &sp->pos, &light_dir);
    if (light_factor <= 0.0001) {
      continue;
    }

    c_alpha = VecDot(unit_normal, light_dir);
    if (c_alpha >= 0) {
      VecScalMul(highlight, 2 * c_alpha, unit_normal);
      VecSub(highlight, highlight, light_dir);
      c_beta = VecDot(highlight, sp->view_vec);

      MakeVector(qd, sd->color.red, sd->color.grn, sd->color.blu);
      VecScalMul(qd, (c_alpha * rd * (1.0 - sd->metalness * sd->smoothness)),
                 qd);

      if (c_beta >= 0) {
        h = 3 / (1.0 - sd->smoothness);
        rn = 1.0 - rd;
        rj = rn + (rn + 0.1) * FR(c_alpha) * GA(c_alpha) * GA(c_gamma);
        if (rj > 1.0) {
          rj = 1.0;
        }
        rs = exp(h * log(c_beta)) * rj;

        MakeVector(qs, sd->color.red - 1.0, sd->color.grn - 1.0,
                   sd->color.blu - 1.0);
        VecScalMul(qs, sd->metalness * (1.0 - FR(c_alpha)), qs);
        qs.x += 1.0;
        qs.y += 1.0;
        qs.z += 1.0;
        VecScalMul(qs, rs, qs);

        VecAdd(qd, qd, qs);
      }

      VecScalMul(qd, light_factor, qd);
      qd.x = lp->color.red * qd.x;
      qd.y = lp->color.grn * qd.y;
      qd.z = lp->color.blu * qd.z;

      col.red += qd.x;
      col.grn += qd.y;
      col.blu += qd.z;
    }
  }

  col.red += sd->ambient * rd * sd->color.red;
  col.grn += sd->ambient * rd * sd->color.grn;
  col.blu += sd->ambient * rd * sd->color.blu;

  color->red = ((col.red > 1.0) ? 1.0 : col.red);
  color->grn = ((col.grn > 1.0) ? 1.0 : col.grn);
  color->blu = ((col.blu > 1.0) ? 1.0 : col.blu);

  opacity->red = sd->opacity.red;
  opacity->grn = sd->opacity.grn;
  opacity->blu = sd->opacity.blu;
}
