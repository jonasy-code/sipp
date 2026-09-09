/*
 * Declarations needed to use noise() and friends...
 */

#ifndef _NOISE_H 
#define _NOISE_H 

#include <geometric.h>

extern bool     noise_ready;

extern void
noise_init(void);

extern double
noise(Vector *v);

extern double
turbulence(Vector *p,
                        int     octaves);

extern Vector
Dnoise(Vector *p);

#endif /* _NOISE_H */
