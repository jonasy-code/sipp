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
 ** filter.h - Filtered versions of the functions that procedural
 **            textures are built from: steps, pulses and square waves
 **            averaged over the area a shading sample covers, so that a
 **            texture can be anti-aliased in the shader instead of by
 **            oversampling.  WIDTH is the size of the sample along the
 **            function's argument (see shade_texture_width() and the
 **            derivatives in a Shade_point); with WIDTH 0.0 every
 **            function is its unfiltered self.
 **/

#ifndef FILTER_H
#define FILTER_H

#include <sipp.h>

/*
 * A step from 0.0 to 1.0 at EDGE, averaged over WIDTH around X: the
 * fraction of the sample that lies above EDGE.
 */
EXTERN double filter_step(double edge, double x, double width);

/*
 * A pulse that is 1.0 between EDGE0 and EDGE1, averaged over WIDTH
 * around X.
 */
EXTERN double filter_pulse(double edge0, double edge1, double x, double width);

/*
 * A square wave of period 2.0 that is +1.0 on [0, 1), -1.0 on [1, 2)
 * and so on, averaged over WIDTH around X.
 */
EXTERN double filter_square_wave(double x, double width);

/*
 * A checkerboard with unit squares: the fraction, of a sample WU wide
 * in U and WV wide in V, that lies on squares where floor(u) + floor(v)
 * is odd.
 */
EXTERN double filter_checker(double u, double v, double wu, double wv);

#endif /* FILTER_H */
