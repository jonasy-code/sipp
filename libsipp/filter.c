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
 ** filter.c - Filtered steps, pulses and square waves.  See filter.h.
 **
 ** Each function is the box filter of its unfiltered form: the average
 ** of the function over an interval of length WIDTH centered on X,
 ** which is the difference of the function's integral at the ends of
 ** the interval, divided by WIDTH.
 **/

#include <math.h>

#include <sipp.h>

#include <filter.h>

double filter_step(double edge, double x, double width) {
  double f;

  if (width <= 0.0) {
    return (x >= edge) ? 1.0 : 0.0;
  }
  f = (x - edge) / width + 0.5;
  if (f <= 0.0) {
    return 0.0;
  }
  if (f >= 1.0) {
    return 1.0;
  }
  return f;
}

double filter_pulse(double edge0, double edge1, double x, double width) {
  return filter_step(edge0, x, width) - filter_step(edge1, x, width);
}

/*
 * The integral of the square wave: a triangle wave, 0 at even
 * integers and 1 at odd ones.
 */
static double triangle_wave(double x) {
  double m = x - 2.0 * floor(x / 2.0); /* x modulo 2 */

  return 1.0 - fabs(m - 1.0);
}

double filter_square_wave(double x, double width) {
  if (width <= 0.0) {
    return (((long)floor(x)) & 1) ? -1.0 : 1.0;
  }
  return (triangle_wave(x + width / 2.0) - triangle_wave(x - width / 2.0)) /
         width;
}

/*
 * The board is the product of the square waves in u and v; it is +1
 * where the parities of floor(u) and floor(v) agree and -1 where they
 * differ.  The average of the product over a box is the product of the
 * averages.
 */
double filter_checker(double u, double v, double wu, double wv) {
  return 0.5 * (1.0 - filter_square_wave(u, wu) * filter_square_wave(v, wv));
}
