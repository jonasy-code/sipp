/*
 * Declarations needed to use noise() and friends...
 */

#ifndef _NOISE_H
#define _NOISE_H

#include <geometric.h>

extern bool noise_ready;

extern void noise_init(void);

extern double noise(Vector *v);

extern double turbulence(Vector *p, int octaves);

extern Vector Dnoise(Vector *p);

/*
 * Band-limited noise, for textures that should not alias.  WIDTH is
 * the size of the area the sample covers, in the same units as the
 * point (see shade_texture_width() in sipp.h; scale it like the point).
 * noise_weight() is how much of a noise octave to keep when a sample
 * covers WIDTH lattice units of it: everything up to one unit, nothing
 * from two units on.  noise_filtered() is noise() times that
 * weight, and turbulence_filtered() is turbulence() with the octaves
 * weighted, and stopped, accordingly.  With WIDTH 0.0 they are the
 * unfiltered functions.
 */
extern double noise_weight(double width);

extern double noise_filtered(Vector *v, double width);

extern double turbulence_filtered(Vector *p, int octaves, double width);

#endif /* _NOISE_H */
