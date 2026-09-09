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
 ** geometric.c - Matrixes, transformations and coordinates.
 **/

#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <smalloc.h>
#include <geometric.h>


/* =================================================================== */
 /* */


Transf_mat   ident_matrix = {{        /* Unit tranfs. matrix */
    { 1.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 1.0 },
    { 0.0, 0.0, 0.0 }
}};


/* =================================================================== */

/*
 * Allocate a new matrix, and if INITMAT != NULL copy the contents
 * of INITMAT to the new matrix,  otherwise copy the identity matrix
 * to the new matrix.
 */

Transf_mat *
transf_mat_create(Transf_mat * initmat)
{
    Transf_mat  * mat;

    mat = (Transf_mat *) smalloc(sizeof(Transf_mat));
    if (initmat != NULL)
	MatCopy(mat, initmat);
    else
	MatCopy(mat, &ident_matrix);

    return mat;
}


void
transf_mat_destruct(Transf_mat * mat)
{
    sfree(mat);
}


/* =================================================================== */
/*          Transformation routines (see also geometric.h)             */


/*
 * Normalize a vector.
 */
void
vecnorm(Vector *vec)
{
    double   len;

    len =  VecLen(*vec);
    if (len == 0.0) {  /* As Mark says, we could really use error handling...*/
        MakeVector(*vec, 0.0, 0.0, 0.0);
    } else {
        VecScalMul(*vec, 1.0 / len, *vec);
    }
}



/*
 * Set MAT to the transformation matrix that represents the 
 * concatenation between the previous transformation in MAT
 * and a translation along the vector described by DX, DY and DZ.
 *
 * [a  b  c  0]   [ 1  0  0  0]     [ a     b     c    0]
 * [d  e  f  0]   [ 0  1  0  0]     [ d     e     f    0]
 * [g  h  i  0] * [ 0  0  1  0]  =  [ g     h     i    0]
 * [j  k  l  1]   [Tx Ty Tz  1]     [j+Tx  k+Ty  l+Tz  1]
 */

void
mat_translate(Transf_mat * mat, double dx, double dy, double dz)
{
    mat->mat[3][0] += dx;
    mat->mat[3][1] += dy;
    mat->mat[3][2] += dz;
}



/*
 * Set MAT to the transformation matrix that represents the 
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the X axis.
 *
 * [a  b  c  0]   [1   0   0  0]     [a  b*Ca-c*Sa  b*Sa+c*Ca  0]
 * [d  e  f  0]   [0  Ca  Sa  0]     [d  e*Ca-f*Sa  e*Sa+f*Ca  0]
 * [g  h  i  0] * [0 -Sa  Ca  0]  =  [g  h*Ca-i*Sa  h*Sa+i*Ca  0]
 * [j  k  l  1]   [0   0   0  1]     [j  k*Ca-l*Sa  k*Se+l*Ca  1]
 */

void
mat_rotate_x(Transf_mat * mat, double ang)
{
    double   cosang;
    double   sinang;
    double   tmp;
    int      i;
    
    cosang = cos(ang);
    sinang = sin(ang);
    if (fabs(cosang) < 1.0e-15) {
        cosang = 0.0;
    }
    if (fabs(sinang) < 1.0e-15) {
        sinang = 0.0;
    }
    for (i = 0; i < 4; ++i) {
	tmp = mat->mat[i][1];
	mat->mat[i][1] = mat->mat[i][1] * cosang
	               - mat->mat[i][2] * sinang;
	mat->mat[i][2] = tmp * sinang + mat->mat[i][2] * cosang;
    }
}



/*
 * Set MAT to the transformation matrix that represents the 
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the Y axis.
 *
 * [a  b  c  0]   [Ca   0 -Sa  0]     [a*Ca+c*Sa  b  -a*Sa+c*Ca  0]
 * [d  e  f  0] * [ 0   1   0  0]  =  [d*Ca+f*Sa  e  -d*Sa+f*Ca  0]
 * [g  h  i  0]   [Sa   0  Ca  0]     [g*Ca+i*Sa  h  -g*Sa+i*Ca  0]
 * [j  k  l  1]   [ 0   0   0  1]     [j*Ca+l*Sa  k  -j*Sa+l*Ca  1]
 */

void
mat_rotate_y(Transf_mat * mat, double ang)
{
    double   cosang;
    double   sinang;
    double   tmp;
    int      i;
    
    cosang = cos(ang);
    sinang = sin(ang);
    if (fabs(cosang) < 1.0e-15) {
        cosang = 0.0;
    }
    if (fabs(sinang) < 1.0e-15) {
        sinang = 0.0;
    }
    for (i = 0; i < 4; ++i) {
	tmp = mat->mat[i][0];
	mat->mat[i][0] = mat->mat[i][0] * cosang
	               + mat->mat[i][2] * sinang;
	mat->mat[i][2] = -tmp * sinang + mat->mat[i][2] * cosang;
    }
}



/*
 * Set MAT to the transformation matrix that represents the 
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the Z axis.
 *
 * [a  b  c  0]   [ Ca  Sa   0  0]     [a*Ca-b*Sa  a*Sa+b*Ca  c  0]
 * [d  e  f  0]   [-Sa  Ca   0  0]     [d*Ca-e*Sa  d*Sa+e*Ca  f  0]
 * [g  h  i  0] * [  0   0   1  0]  =  [g*Ca-h*Sa  g*Sa+h*Ca  i  0]
 * [j  k  l  1]   [  0   0   0  1]     [j*Ca-k*Sa  j*Sa+k*Ca  l  0]
 */

void
mat_rotate_z(Transf_mat * mat, double ang)
{
    double   cosang;
    double   sinang;
    double   tmp;
    int      i;
    
    cosang = cos(ang);
    sinang = sin(ang);
    if (fabs(cosang) < 1.0e-15) {
        cosang = 0.0;
    }
    if (fabs(sinang) < 1.0e-15) {
        sinang = 0.0;
    }
    for (i = 0; i < 4; ++i) {
	tmp = mat->mat[i][0];
	mat->mat[i][0] = mat->mat[i][0] * cosang
	               - mat->mat[i][1] * sinang;
	mat->mat[i][1] = tmp * sinang + mat->mat[i][1] * cosang;
    }
}



/*
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the line represented
 * by the point POINT and the vector VECTOR.
 */

void
mat_rotate(Transf_mat * mat, Vector * point, Vector * vector, double ang)
{
    double   ang2;
    double   ang3;

    ang2 = atan2(vector->y, vector->x);
    ang3 = atan2(hypot(vector->x, vector->y), vector->z);
    mat_translate(mat, -point->x, -point->y, -point->z);
    mat_rotate_z(mat, -ang2);
    mat_rotate_y(mat, -ang3);
    mat_rotate_z(mat, ang);
    mat_rotate_y(mat, ang3);
    mat_rotate_z(mat, ang2);
    mat_translate(mat, point->x, point->y, point->z);
}



/*
 * Set MAT to the transformation matrix that represents the 
 * concatenation between the previous transformation in MAT
 * and a scaling with the scaling factors XSCALE, YSCALE and ZSCALE
 *
 * [a  b  c  0]   [Sx  0  0  0]     [a*Sx  b*Sy  c*Sz  0]
 * [d  e  f  0]   [ 0 Sy  0  0]     [d*Sx  e*Sy  f*Sz  0]
 * [g  h  i  0] * [ 0  0 Sz  0]  =  [g*Sx  h*Sy  i*Sz  0]
 * [j  k  l  1]   [ 0  0  0  1]     [j*Sx  k*Sy  l*Sz  1]
 */

void
mat_scale(Transf_mat * mat, double xscale, double yscale, double zscale)
{
    int   i;

    for (i = 0; i < 4; ++i) {
	mat->mat[i][0] *= xscale;
	mat->mat[i][1] *= yscale;
	mat->mat[i][2] *= zscale;
    }
}



/*
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a mirroring in the plane defined by the point POINT
 * and the normal vector NORM.
 */

void
mat_mirror_plane(Transf_mat * mat, Vector * point, Vector * norm)
{
    Transf_mat   tmp;
    double   factor;

    /* The first thing we do is to make a transformation matrix */
    /* for mirroring through a plane with the same normal vector */
    /* as our, but through the origin instead. */
    factor = 2.0 / (norm->x * norm->x + norm->y * norm->y 
		    + norm->z * norm->z);
    
    /* The diagonal elements. */
    tmp.mat[0][0] = 1 - factor * norm->x * norm->x;
    tmp.mat[1][1] = 1 - factor * norm->y * norm->y;
    tmp.mat[2][2] = 1 - factor * norm->z * norm->z;
    
    /* The rest of the matrix */
    tmp.mat[1][0] = tmp.mat[0][1] = -factor * norm->x * norm->y;
    tmp.mat[2][0] = tmp.mat[0][2] = -factor * norm->x * norm->z;
    tmp.mat[2][1] = tmp.mat[1][2] = -factor * norm->y * norm->z;
    tmp.mat[3][0] = tmp.mat[3][1] = tmp.mat[3][2] = 0.0;

    /* Do the actual transformation. This is done in 3 steps: */
    /* 1) Translate the plane so that it goes through the origin. */
    /* 2) Do the actual mirroring. */
    /* 3) Translate it all back to the starting position. */
    mat_translate(mat, -point->x, -point->y, -point->z);
    mat_mul(mat, mat, &tmp);
    mat_translate(mat, point->x, point->y, point->z);
}



/*
 * Multiply the Matrix A with the Matrix B, and store the result
 * into the Matrix RES. It is possible for RES to point to the 
 * same Matrix as either A or B since the result is stored into
 * a temporary during computation.
 *
 * [a b c 0]  [A B C 0]     [aA+bD+cG    aB+bE+cH    aC+bF+cI    0]
 * [d e f 0]  [D E F 0]     [dA+eD+fG    dB+eE+fH    dC+eF+fI    0]
 * [g h i 0]  [G H I 0]  =  [gA+hD+iG    gB+hE+iH    gC+hF+iI    0]
 * [j k l 1]  [J K L 1]     [jA+kD+lG+J  jB+kE+lH+K  jC+kF+lI+L  1]
 */

void
mat_mul(Transf_mat * res, Transf_mat * a, Transf_mat * b)
{
    Transf_mat   tmp;
    int      i;

    for (i = 0; i < 4; ++i) {
	tmp.mat[i][0] = a->mat[i][0] * b->mat[0][0]
	              + a->mat[i][1] * b->mat[1][0] 
	              + a->mat[i][2] * b->mat[2][0];
	tmp.mat[i][1] = a->mat[i][0] * b->mat[0][1]
  	              + a->mat[i][1] * b->mat[1][1] 
	              + a->mat[i][2] * b->mat[2][1];
	tmp.mat[i][2] = a->mat[i][0] * b->mat[0][2]
 	              + a->mat[i][1] * b->mat[1][2]
	              + a->mat[i][2] * b->mat[2][2];
    }

    tmp.mat[3][0] += b->mat[3][0];
    tmp.mat[3][1] += b->mat[3][1];
    tmp.mat[3][2] += b->mat[3][2];

    MatCopy(res, &tmp);
}



/*
 * Transform the Point3d VEC with the transformation matrix MAT, and
 * put the result into the vector *RES.
 *
 *               [a  b  c  0]
 *               [d  e  f  0]
 * [x  y  z  1]  [g  h  i  0]  =  [ax+dy+gz+j  bx+ey+hz+k  cx+fy+iz+l  1]
 *               [j  k  l  1]
 */

void
point_transform(Vector * res, Vector * vec, Transf_mat * mat)
{
    res->x = mat->mat[0][0] * vec->x + mat->mat[1][0] * vec->y 
        + mat->mat[2][0] * vec->z + mat->mat[3][0];
    res->y = mat->mat[0][1] * vec->x + mat->mat[1][1] * vec->y 
        + mat->mat[2][1] * vec->z + mat->mat[3][1];
    res->z = mat->mat[0][2] * vec->x + mat->mat[1][2] * vec->y 
        + mat->mat[2][2] * vec->z + mat->mat[3][2];
}
