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
 ** viewpoint.h - Types and interface to viewpoint.c
 **/

#ifndef VIEWPOINT_H
#define VIEWPOINT_H

#include <geometric.h>
#include <sipp.h>

#define ZCLIPF 100.0 /* Magic number used when defining hither & yon */

extern Camera *sipp_current_camera; /* Viewpoint of the scene  */
extern double hither;               /* Hither z-clipping plane */
extern double yon;                  /* Yonder z-clipping plane */

extern void camera_init(void);

extern void get_view_transf(Transf_mat *view_mat, Camera *camera,
                            Render_mode render_mode);

#endif /* VIEWPOINT_H */
