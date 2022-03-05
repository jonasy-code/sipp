/*
 * Declarations needed to use noise() and friends...
 */

#ifndef _NOISE_H 
#define _NOISE_H 

#include <geometric.h>

extern bool     noise_ready;

extern void
noise_init _ANSI_ARGS_((void));

extern double
noise _ANSI_ARGS_((Vector *v));

extern double
turbulence _ANSI_ARGS_((Vector *p,
                        int     octaves));

extern Vector
Dnoise _ANSI_ARGS_((Vector *p));

#endif /* _NOISE_H */
