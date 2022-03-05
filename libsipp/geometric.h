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
 ** geometric.h - All kinds of stuff with matrixes, transformations, 
 **               coordinates
 **/

/* Make sure no multiple including */
#ifndef _GEOMETRIC_H_
#define _GEOMETRIC_H_

#include <math.h>
#ifndef NOMEMCPY
#include <memory.h>
#endif


/* #define PI    3.1415926535897932384626 */


typedef struct {
    double x, y, z;
} Vector;


/*
 * NOTE:
 * Capitalized types denote Vectors and other aggregates.
 * Lower case denote scalars.
 */

/* V = Vec(x, y, z) */
#define MakeVector(V, xx, yy, zz)  { (V).x=(xx); \
                                     (V).y=(yy); \
                                     (V).z=(zz); }

/* A = -A */
#define VecNegate(A)	         { (A).x=0-(A).x; \
			           (A).y=0-(A).y; \
			           (A).z=0-(A).z; }

/* return A . B */
#define VecDot(A, B)	((A).x*(B).x+(A).y*(B).y+(A).z*(B).z)

/* return length(A) */
#define VecLen(A)	(sqrt((double)VecDot(A, A)))

/* B = A */
#define VecCopy(B, A)	((B) = (A))

/* C = A + B */
#define VecAdd(C, A, B)	 { (C).x=(A).x+(B).x; \
			   (C).y=(A).y+(B).y; \
			   (C).z=(A).z+(B).z; }

/* C = A - B */
#define VecSub(C, A, B)	 { (C).x=(A).x-(B).x; \
			   (C).y=(A).y-(B).y; \
			   (C).z=(A).z-(B).z; }

/* C = a*A */
#define VecScalMul(C, a, A)	 { (C).x=(a)*(A).x; \
				   (C).y=(a)*(A).y; \
				   (C).z=(a)*(A).z; }

/* C = a*A + B */
#define VecAddS(C, a, A, B)	 { (C).x=(a)*(A).x+(B).x; \
				   (C).y=(a)*(A).y+(B).y; \
				   (C).z=(a)*(A).z+(B).z; }

/* C = a*A + b*B */
#define VecComb(C, a, A, b, B)	 { (C).x=(a)*(A).x+(b)*(B).x; \
				   (C).y=(a)*(A).y+(b)*(B).y; \
			 	   (C).z=(a)*(A).z+(b)*(B).z; }

/* C = A X B */
#define VecCross(C, A, B)   	 { (C).x=(A).y*(B).z-(A).z*(B).y; \
                                   (C).y=(A).z*(B).x-(A).x*(B).z; \
			           (C).z=(A).x*(B).y-(A).y*(B).x; }


#define VecMax(C, A, B)        	 { (C).x=(((A).x>(B).x)?(A).x:(B).x); \
                                   (C).y=(((A).y>(B).y)?(A).y:(B).y); \
                                   (C).z=(((A).z>(B).z)?(A).z:(B).z); }


#define VecMin(C, A, B)        	 { (C).x=(((A).x<(B).x)?(A).x:(B).x); \
                                   (C).y=(((A).y<(B).y)?(A).y:(B).y); \
                                   (C).z=(((A).z<(B).z)?(A).z:(B).z); }

/* ================================================================ */
/*                         Matrix operations                        */


/*
 * Define a homogenous transformation matrix. The first row (vector) 
 * is the new X axis, i.e. the X axis in the transformed coordinate 
 * system. The second row is the new Y axis, and so on. The last row
 * is the translation, for a transformed point.
 *
 * The reason we make surround the rows with a struct is that we
 * don't want to say (Transf_mat *) &foo[0] instead of &foo when
 * sending an address to a matrix as a parameter to a function.
 * Alas, arrays are not first class objects in C.
 */

typedef struct {
    double   mat[4][3];
} Transf_mat;


extern Transf_mat   ident_matrix;


/* *A = *B    N.b. A and B are pointers! */
#define MatCopy(A, B)		 (*A) = (*B)


/*----------------------------------------------------------------------*/


/* Function declarations for the functions in geometric.c */

EXTERN void
vecnorm _ANSI_ARGS_((Vector  *vec));

EXTERN Transf_mat *
transf_mat_create _ANSI_ARGS_((Transf_mat  *initmat));

EXTERN void
transf_mat_destruct _ANSI_ARGS_((Transf_mat  *mat));

EXTERN void
mat_translate _ANSI_ARGS_((Transf_mat  *mat,
                           double       dx,
                           double       dy, 
                           double       dz));

EXTERN void
mat_rotate_x _ANSI_ARGS_((Transf_mat  *mat,
                          double       ang));

EXTERN void
mat_rotate_y _ANSI_ARGS_((Transf_mat  *mat,
                          double       ang));

EXTERN void
mat_rotate_z _ANSI_ARGS_((Transf_mat  *mat,
                          double       ang));

EXTERN void
mat_rotate _ANSI_ARGS_((Transf_mat  *mat,
                        Vector      *point,
                        Vector      *vector,
                        double       ang));

EXTERN void
mat_scale _ANSI_ARGS_((Transf_mat  *mat,
                       double       xscale,
                       double       yscale,
                       double       zscale));

EXTERN void
mat_mirror_plane _ANSI_ARGS_((Transf_mat  *mat,
                              Vector      *point,
                              Vector      *norm));

EXTERN void
mat_mul _ANSI_ARGS_((Transf_mat  *res,
                     Transf_mat  *a,
                     Transf_mat  *b));

EXTERN void
point_transform _ANSI_ARGS_((Vector      *res,
                             Vector      *vec,
                             Transf_mat  *mat));


#endif  /* _GEOMETRIC_H_ */
