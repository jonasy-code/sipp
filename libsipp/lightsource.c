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
 ** lightsource.c - Functions that handles lightsources.
 **/

#include <math.h>
#include <stdio.h>

#ifndef MAXFLOAT
#define MAXFLOAT ((float)3.40282346638528860e+38)
#endif

#include <lightsource.h>
#include <sipp.h>
#include <smalloc.h>

Lightsource *lightsrc_stack; /* Stack of installed lightsources. */

extern int depthmap_size;

/*
 * Index into the jitter table for shadow sampling.  Each thread walks
 * the table on its own, so the pattern a pixel gets depends on which
 * thread renders it; with one thread it is the same as it always was.
 */
static _Thread_local int rand_index = 0;
static double rand_no[256];

static double shadow_sample(Shadow_info *sh, const Vector *pos);

/*
 * Restart the jitter sequence used for soft shadows.  The renderer
 * calls this at the start of every band, so that the shadow pattern
 * depends only on how the image is divided into bands, not on which
 * thread happened to render what.
 */
void shadow_jitter_reset(void) { rand_index = 0; }

/*
 * Create a new lightsource in the scene.
 */
Lightsource *lightsource_create(double x, double y, double z, double red,
                                double grn, double blu, Light_type type) {
  Lightsource *lp;
  Dir_light_info *ip;

  if (type != LIGHT_DIRECTION && type != LIGHT_POINT) {
    return NULL;
  }

  lp = (Lightsource *)scalloc(1, sizeof(Lightsource));
  ip = (Dir_light_info *)scalloc(1, sizeof(Dir_light_info));

  MakeVector(ip->dir, x, y, z);
  if (type == LIGHT_DIRECTION) {
    vecnorm(&ip->dir);
  }
  lp->info = (void *)ip;

  lp->type = type;
  lp->color.red = red;
  lp->color.grn = grn;
  lp->color.blu = blu;
  lp->active = TRUE;
  lp->next = lightsrc_stack;
  lp->shadow.active = FALSE;
  lightsrc_stack = lp;

  return lp;
}

/*
 * Change the position of a lightsource. If it is a directional
 * lightsource it's direction is changed.
 */
void lightsource_put(Lightsource *lp, double x, double y, double z) {
  if (lp->type != LIGHT_DIRECTION && lp->type != LIGHT_POINT) {
    return;
  }

  MakeVector(((Dir_light_info *)(lp->info))->dir, x, y, z);
  if (lp->type == LIGHT_DIRECTION) {
    vecnorm(&((Dir_light_info *)(lp->info))->dir);
  }
}

/*
 * Define a new spotlight in the scene.
 */
Lightsource *spotlight_create(double x, double y, double z, double to_x,
                              double to_y, double to_z, double fov, double red,
                              double grn, double blu, Light_type type,
                              bool shadows) {
  Lightsource *lp;
  Spot_light_info *sp;

  if (type != SPOT_SHARP && type != SPOT_SOFT) {
    return NULL;
  }

  lp = (Lightsource *)scalloc(1, sizeof(Lightsource));
  sp = (Spot_light_info *)scalloc(1, sizeof(Spot_light_info));

  MakeVector(sp->pos, x, y, z);
  MakeVector(sp->point, to_x, to_y, to_z);
  VecSub(sp->dir, sp->point, sp->pos);
  vecnorm(&sp->dir);
  sp->cos_fov = cos(fov * M_PI / 180.0 * 0.5);
  lp->info = (void *)sp;

  lp->shadow.fov_factor = tan(fov * M_PI / 180.0 * 0.5);
  lp->shadow.active = shadows;
  lp->shadow.d_map = NULL;

  lp->type = type;
  lp->color.red = red;
  lp->color.grn = grn;
  lp->color.blu = blu;
  lp->active = TRUE;
  lp->next = lightsrc_stack;
  lightsrc_stack = lp;

  return lp;
}

/*
 * Remove the lightsource LIGHT from the list of lights and free all
 * resources attached to it.
 */
void light_destruct(Lightsource *light) {
  Lightsource *lght;

  /* First,  we must remove the light from the list of active lights. */
  while (lightsrc_stack == light)
    lightsrc_stack = lightsrc_stack->next;

  lght = lightsrc_stack;
  while (lght != NULL) {
    if (lght->next == light)
      lght->next = lght->next->next;
    else
      lght = lght->next;
  }

  sfree(light->info);
  sfree(light);
}

/*
 * Change the position of a spotlight.
 */
void spotlight_pos(Lightsource *lp, double x, double y, double z) {
  Spot_light_info *sp;

  if (lp->type != SPOT_SOFT && lp->type != SPOT_SHARP) {
    return;
  }

  sp = (Spot_light_info *)(lp->info);
  MakeVector(sp->pos, x, y, z);
  VecSub(sp->dir, sp->point, sp->pos);
  vecnorm(&sp->dir);
}

/*
 * Change the point a spotlight is shining at.
 */
void spotlight_at(Lightsource *lp, double x, double y, double z) {
  Spot_light_info *sp;

  if (lp->type != SPOT_SOFT && lp->type != SPOT_SHARP) {
    return;
  }

  sp = (Spot_light_info *)(lp->info);
  MakeVector(sp->point, x, y, z);
  VecSub(sp->dir, sp->point, sp->pos);
  vecnorm(&sp->dir);
}

/*
 * Change the opening angle of a spotlight.
 */
void spotlight_opening(Lightsource *lp, double fov) {
  if (lp->type != SPOT_SOFT && lp->type != SPOT_SHARP) {
    return;
  }

  ((Spot_light_info *)(lp->info))->cos_fov = cos(fov * M_PI / 180.0 * 0.5);
}

/*
 * Turn on or off shadow generation from a spotlight.
 */
void spotlight_shadows(Lightsource *lp, bool flag) {
  if (lp->type != SPOT_SOFT && lp->type != SPOT_SHARP) {
    return;
  }

  lp->shadow.active = flag;
}

/*
 * Set the color of the light from a lightsource or a spotlight.
 */
void light_color(Lightsource *lp, double red, double grn, double blu) {
  lp->color.red = red;
  lp->color.grn = grn;
  lp->color.blu = blu;
}

/*
 * Turn a lightsource or spotlight on or off.
 */
void light_active(Lightsource *lp, bool flag) { lp->active = flag; }

/*
 * Evaluate how much light from the lightsource LP thas is falling
 * on the point POS. Type of lightsource and shadows are taken into
 * account.
 * In VEC we return a vector pointing from POS to LP.
 */
double light_eval(Lightsource *lp, const Vector *pos, Vector *vec) {
  double fov_factor;

  switch (lp->type) {
  case LIGHT_DIRECTION:
    VecCopy(*vec, ((Dir_light_info *)(lp->info))->dir);
    return 1.0;

  case LIGHT_POINT:
    VecSub(*vec, ((Dir_light_info *)(lp->info))->dir, *pos);
    vecnorm(vec);
    return 1.0;

  case SPOT_SOFT:
    VecSub(*vec, *pos, ((Spot_light_info *)(lp->info))->pos);
    vecnorm(vec);

    fov_factor = VecDot(*vec, ((Spot_light_info *)(lp->info))->dir);
    if (fov_factor <= 0.0 ||
        fov_factor < ((Spot_light_info *)(lp->info))->cos_fov) {
      VecNegate(*vec);
      return 0.0;

    } else {
      fov_factor = (cos((1.0 - fov_factor) * M_PI /
                        (1.0 - ((Spot_light_info *)(lp->info))->cos_fov)) *
                        0.5 +
                    0.5);
      if (lp->shadow.active && lp->shadow.d_map) {
        VecNegate(*vec);
        return fov_factor * shadow_sample(&lp->shadow, pos);

      } else {
        VecNegate(*vec);
        return fov_factor;
      }
    }

  case SPOT_SHARP:
    VecSub(*vec, *pos, ((Spot_light_info *)(lp->info))->pos);
    vecnorm(vec);

    if (VecDot(*vec, ((Spot_light_info *)(lp->info))->dir) <
        ((Spot_light_info *)(lp->info))->cos_fov) {
      VecNegate(*vec);
      return 0.0;

    } else if (lp->shadow.active && lp->shadow.d_map) {
      VecNegate(*vec);
      return shadow_sample(&lp->shadow, pos);

    } else {
      VecNegate(*vec);
      return 1.0;
    }
  }

  /* Unknown lightsource type; should never happen. */
  return 0.0;
}

/*
 * Sample a depth map and do percentage closer filtering to
 * see how much shadowed POS is.
 */
static const double BOXRES = 0.002;
static double shadow_sample(Shadow_info *sh, const Vector *pos) {
  Vector lp_view;
  int lit;
  int ns;
  int x, y;
  int i, j;
  double lu, hu;
  double lv, hv;
  double xmin, ymin;
  double boxres;
  double ds, js;
  double persp;

  point_transform(&lp_view, pos, &sh->matrix);
  persp = lp_view.z * sh->fov_factor;
  lp_view.x = (lp_view.x * depthmap_size * 0.5 / persp + depthmap_size * 0.5);
  lp_view.y = (lp_view.y * depthmap_size * 0.5 / persp + depthmap_size * 0.5);

  boxres = BOXRES * depthmap_size;
  lu = floor(lp_view.x - boxres);
  lv = floor(lp_view.y - boxres);
  hu = ceil(lp_view.x + boxres);
  hv = ceil(lp_view.y + boxres);
  if (lu >= depthmap_size || hu < 0.0 || lv >= depthmap_size || hv < 0.0) {
    return 1.0;
  }

  ns = (int)(boxres * 2.0 + 0.5);

  ds = 2.0 * boxres / ns;
  js = ds * 0.5;

  xmin = lp_view.x - boxres + js;
  ymin = lp_view.y - boxres + js;

  lit = ns * ns;
  for (i = 0, lp_view.x = xmin; i < ns; i++, lp_view.x += ds) {
    for (j = 0, lp_view.y = ymin; j < ns; j++, lp_view.y += ds) {
      rand_index = (rand_index + 1) & 255;
      x = lp_view.x + rand_no[rand_index] * js;
      rand_index = (rand_index + 1) & 255;
      y = lp_view.y + rand_no[rand_index] * js;
      if (x >= 0 && x < depthmap_size && y >= 0 && y < depthmap_size) {
        if (lp_view.z >
            (sh->d_map[(depthmap_size - 1 - y) * depthmap_size + x] +
             sh->bias)) {
          lit--;
        }
      }
    }
  }

  return (double)lit / (double)(ns * ns);
}

/*
 * Create and initialize depthmaps for all lightsources
 * that have their shadow generation activated.
 */
void depthmaps_create(void) {
  Lightsource *lp;
  int i;

  for (i = 0; i < 256; i++) {
    rand_no[i] = (RANDOM() + 1.0) * 0.5;
  }
  rand_index = 0;
  for (lp = lightsrc_stack; lp != NULL; lp = lp->next) {
    if (lp->shadow.active) {
      if (NULL != lp->shadow.d_map) {
        sfree(lp->shadow.d_map);
      }
      lp->shadow.d_map =
          (float *)smalloc(depthmap_size * depthmap_size * sizeof(float));
      for (i = 0; i < depthmap_size * depthmap_size; i++) {
        lp->shadow.d_map[i] = MAXFLOAT;
      }
    }
  }
}

/*
 * Release the memory used by the depthmaps.
 */
void depthmaps_destruct(void) {
  Lightsource *lp;

  for (lp = lightsrc_stack; lp != NULL; lp = lp->next) {
    if (lp->shadow.active && lp->shadow.d_map != NULL) {
      sfree(lp->shadow.d_map);
      lp->shadow.d_map = NULL;
    }
  }
}
