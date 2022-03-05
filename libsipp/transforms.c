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
 ** transforms.c - Functions that handles object transformations.
 **/

#include <stdio.h>

#include <sipp.h>


/*
 * Set the transformation matrix of OBJ to MATRIX.
 */
void
object_set_transf(obj, matrix)
    Object     *obj;
    Transf_mat *matrix;
{
    MatCopy(&obj->transf, matrix);
}


/*
 * Retrieve the transformation matrix of OBJ
 */
Transf_mat *
object_get_transf(obj, matrix)
    Object     *obj;
    Transf_mat *matrix;
{
    Transf_mat *tmp;

    if (matrix != NULL) {
        MatCopy(matrix, &obj->transf);
        return matrix;
    } else {
        tmp = transf_mat_create(NULL);
        MatCopy(tmp, &obj->transf);
        return tmp;
    }
}


/*
 * Set the transformation matrix of OBJ to the identity matrix.
 */
void
object_clear_transf(obj)
    Object *obj;
{
    MatCopy(&obj->transf, &ident_matrix);
}


/*
 * Post multiply MATRIX into the transformation matrix of OBJ.
 */
void
object_transform(obj, matrix)
    Object     *obj;
    Transf_mat *matrix;
{
    mat_mul(&obj->transf, &obj->transf, matrix);
}


/*
 * Rotate the object OBJ ANG radians about the x-axis.
 */
void
object_rot_x(obj, ang)
    Object *obj;
    double  ang;
{
    mat_rotate_x(&obj->transf, ang);
}


/*
 * Rotate the object OBJ ANG radians about the y-axis.
 */
void
object_rot_y(obj, ang)
    Object *obj;
    double  ang;
{
    mat_rotate_y(&obj->transf, ang);
}


/*
 * Rotate the object OBJ ANG radians about the z-axis.
 */
void
object_rot_z(obj, ang)
    Object *obj;
    double  ang;
{
    mat_rotate_z(&obj->transf, ang);
}


/*
 * Rotate the object OBJ ANG radians about the line defined
 * by POINT and VEC.
 */
void
object_rot(obj, point, vec, ang)
    Object *obj;
    Vector *point;
    Vector *vec;
    double  ang;
{
    mat_rotate(&obj->transf, point, vec, ang);
}


/*
 * Scale the object OBJ with respect to the origin.
 */
void
object_scale(obj, xscale, yscale, zscale)
    Object *obj;
    double  xscale, yscale, zscale;
{
    mat_scale(&obj->transf, xscale, yscale, zscale);
}


/*
 * Translate the object OBJ.
 */
void
object_move(obj, dx, dy, dz)
    Object *obj;
    double  dx, dy, dz;
{
    mat_translate(&obj->transf, dx, dy, dz);
}




