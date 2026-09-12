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
 ** viewpoint.c - Functions that handles the viewpoint definition and
 **               calculation of viewing transformation.
 **/

#include <sipp.h>
#include <smalloc.h>
#include <viewpoint.h>

Camera *sipp_current_camera; /* Current camera used as viewpoint */
Camera *sipp_camera;         /* Pointer to internal camera */
static Camera intern_camera; /* Internal camera */

/*
 * Constants used in viewing transformation.
 */
double hither; /* Hither z-clipping plane */
double yon;    /* Yonder z-clipping plane */

/*
 * Initialize the internal camera.
 */
void camera_init(void) {
  sipp_camera = &intern_camera;
  camera_params(sipp_camera, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                0.25);
  camera_use(sipp_camera);
}

/*
 * Create a virtual camera.
 */
Camera *camera_create(void) {
  Camera *cp;

  cp = (Camera *)smalloc(sizeof(Camera));
  camera_params(cp, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.25);
  camera_clipping(cp, 0.0, 0.0);
  return cp;
}

void camera_clipping(Camera *cp, double hither_plane, double yon_plane) {
  if (hither_plane > 0.0 && yon_plane > hither_plane) {
    cp->hither = hither_plane;
    cp->yon = yon_plane;
  } else {
    cp->hither = 0.0;
    cp->yon = 0.0;
  }
}

/*
 * Return memory used by a virtual camera.
 */
void camera_destruct(Camera *cp) {
  if (cp != sipp_camera) {
    sfree(cp);
    if (cp == sipp_current_camera) {
      camera_use(sipp_camera);
    }
  }
}

/*
 * Change the position of a virtual camera.
 */
void camera_position(Camera *cp, double x, double y, double z) {
  MakeVector(cp->position, x, y, z);
}

/*
 * Change the point a virtual camera is "looking" at.
 */
void camera_look_at(Camera *cp, double x, double y, double z) {
  MakeVector(cp->lookat, x, y, z);
}

/*
 * Set the up vector of a virtual camera.
 */
void camera_up(Camera *cp, double x, double y, double z) {
  MakeVector(cp->up, x, y, z);
}

/*
 * Set the focal factor of a virtual camera.
 */
void camera_focal(Camera *cp, double focal) { cp->focal_ratio = focal; }

/*
 * Set all parameters in a virtual camera in a single call.
 */
void camera_params(Camera *cp, double x0, double y0, double z0, double x,
                   double y, double z, double ux, double uy, double uz,
                   double ratio) {
  MakeVector(cp->position, x0, y0, z0);
  MakeVector(cp->lookat, x, y, z);
  MakeVector(cp->up, ux, uy, uz);
  cp->focal_ratio = ratio;
}

/*
 * Set the current viewpoint parameters to be
 * the ones in the virtyal camera pointed to by CP.
 */
void camera_use(Camera *cp) { sipp_current_camera = cp; }

/*
 * Build a transformation matrix for transformation
 * into view coordinates from a particular camera position.
 */
void get_view_transf(Transf_mat *view_mat, Camera *camera,
                     Render_mode render_mode) {
  Vector tmp;
  double transl[3];
  double vy, vz;
  int i, j;

  /*
   * We need a translation so the origo
   * of the view coordinate system is placed
   * in the viewpoint.
   */
  transl[0] = -camera->position.x;
  transl[1] = -camera->position.y;
  transl[2] = -camera->position.z;

  /*
   * The clipping planes: the camera's own, or else heuristic values
   * from the distance to the looked at point.
   */
  VecSub(tmp, camera->lookat, camera->position);
  if (camera->hither > 0.0) {
    hither = camera->hither;
    yon = camera->yon;
  } else {
    hither = VecLen(tmp) / ZCLIPF;
    yon = VecLen(tmp) * ZCLIPF;
  }

  /*
   * Then we need a rotation that makes the
   * up-vector point up, and alignes the sightline
   * with the z-axis.
   * This code might seem magic but the algebra behind
   * it can be found in Jim Blinn's Corner in IEEE CG&A July 1988
   */
  vecnorm(&tmp);
  vecnorm(&camera->up);
  vz = VecDot(tmp, camera->up);
  if ((vz * vz) > 1.0) { /* this should not happen, but... */
    vz = 1.0;
  }
  vy = sqrt(1.0 - vz * vz);
  if (vy == 0.0) { /* oops, the world collapses... */
    vy = 1.0e10;
    vz = 1.0;
  } else {
    vy = 1.0 / vy;
  }

  view_mat->mat[0][2] = tmp.x;
  view_mat->mat[1][2] = tmp.y;
  view_mat->mat[2][2] = tmp.z;

  VecScalMul(tmp, vz, tmp);
  VecSub(tmp, camera->up, tmp);
  view_mat->mat[0][1] = tmp.x * vy;
  view_mat->mat[1][1] = tmp.y * vy;
  view_mat->mat[2][1] = tmp.z * vy;

  view_mat->mat[0][0] = (view_mat->mat[1][1] * view_mat->mat[2][2] -
                         view_mat->mat[1][2] * view_mat->mat[2][1]);
  view_mat->mat[1][0] = (view_mat->mat[2][1] * view_mat->mat[0][2] -
                         view_mat->mat[2][2] * view_mat->mat[0][1]);
  view_mat->mat[2][0] = (view_mat->mat[0][1] * view_mat->mat[1][2] -
                         view_mat->mat[0][2] * view_mat->mat[1][1]);

  /*
   * Install the translation into the matrix.
   * Note that it is PRE-multiplied into the matrix.
   */
  for (i = 0; i < 3; i++) {
    view_mat->mat[3][i] = 0.0;
    for (j = 0; j < 3; j++) {
      view_mat->mat[3][i] += transl[j] * view_mat->mat[j][i];
    }
  }

  /*
   * Since the screen coordinates are defined in a left handed
   * coordinate system, we must switch the sign of the first
   * column in the matrix to get appropriate signs of x-values
   * unless LINE rendering is used. In that case we switch the
   * y-axis also to get the origin in the image in the upper left.
   */
  if (render_mode == LINE) {
    view_mat->mat[0][1] = -view_mat->mat[0][1];
    view_mat->mat[1][1] = -view_mat->mat[1][1];
    view_mat->mat[2][1] = -view_mat->mat[2][1];
    view_mat->mat[3][1] = -view_mat->mat[3][1];
  }
  view_mat->mat[0][0] = -view_mat->mat[0][0];
  view_mat->mat[1][0] = -view_mat->mat[1][0];
  view_mat->mat[2][0] = -view_mat->mat[2][0];
  view_mat->mat[3][0] = -view_mat->mat[3][0];
}
