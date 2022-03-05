#include <math.h>

#include <sipp.h>
#include <noise.h>

/* ================================================================ */
/*                      Noise and friends                           */


/*
 *	Noise and Dnoise routines
 *
 *	Many thanks to Jon Buller of Colorado State University for these
 *	routines.
 */


#define NUMPTS	512
#define P1	173
#define P2	263
#define P3	337
#define phi	0.6180339


bool noise_ready = FALSE;

static double pts[NUMPTS];

static double
iadjust _ANSI_ARGS_((double f));

/*
 * Doubles might have values that is too large to be
 * stored in an int. If this is the case we "fold" the
 * value about MAXINT or MININT until it is of an
 * acceptable size.
 *
 * NOTE: 32 bit integers is assumed.
 */
static double
iadjust(f)
    double f;
{
    while ((f > 2147483647.0) || (f < -2147483648.0)) {
        if (f > 2147483647.0) {         /* 2**31 - 1 */
            f = (4294967294.0 - f);     /* 2**32 - 2 */
        } else if (f < -2147483648.0){
            f = (-4294967296.0 - f);
        }
    }
    return f;
}



/*
 * Initialize the array of random numbers.
 * RANDOM() is defined in sipp.h
 */
void noise_init()
{
    int i;
   
    for (i = 0; i < NUMPTS; ++i)
        pts[i] = RANDOM();
    noise_ready = TRUE;
}



double noise(v)
Vector *v;
{
   Vector p;
   int xi, yi, zi;
   int xa, xb, xc, ya, yb, yc, za, zb, zc;
   double xf, yf, zf;
   double x2, x1, x0, y2, y1, y0, z2, z1, z0;
   double p000, p100, p200, p010, p110, p210, p020, p120, p220;
   double p001, p101, p201, p011, p111, p211, p021, p121, p221;
   double p002, p102, p202, p012, p112, p212, p022, p122, p222;
   double tmp;

   p = *v;

   p.x = iadjust(p.x);
   p.y = iadjust(p.y);
   p.z = iadjust(p.z);
   xi = floor(p.x);
   xa = floor(P1 * (xi * phi - floor(xi * phi)));
   xb = floor(P1 * ((xi + 1) * phi - floor((xi + 1) * phi)));
   xc = floor(P1 * ((xi + 2) * phi - floor((xi + 2) * phi)));

   yi = floor(p.y);
   ya = floor(P2 * (yi * phi - floor(yi * phi)));
   yb = floor(P2 * ((yi + 1) * phi - floor((yi + 1) * phi)));
   yc = floor(P2 * ((yi + 2) * phi - floor((yi + 2) * phi)));

   zi = floor(p.z);
   za = floor(P3 * (zi * phi - floor(zi * phi)));
   zb = floor(P3 * ((zi + 1) * phi - floor((zi + 1) * phi)));
   zc = floor(P3 * ((zi + 2) * phi - floor((zi + 2) * phi)));

   p000 = pts[(xa + ya + za) % NUMPTS];
   p100 = pts[(xb + ya + za) % NUMPTS];
   p200 = pts[(xc + ya + za) % NUMPTS];
   p010 = pts[(xa + yb + za) % NUMPTS];
   p110 = pts[(xb + yb + za) % NUMPTS];
   p210 = pts[(xc + yb + za) % NUMPTS];
   p020 = pts[(xa + yc + za) % NUMPTS];
   p120 = pts[(xb + yc + za) % NUMPTS];
   p220 = pts[(xc + yc + za) % NUMPTS];
   p001 = pts[(xa + ya + zb) % NUMPTS];
   p101 = pts[(xb + ya + zb) % NUMPTS];
   p201 = pts[(xc + ya + zb) % NUMPTS];
   p011 = pts[(xa + yb + zb) % NUMPTS];
   p111 = pts[(xb + yb + zb) % NUMPTS];
   p211 = pts[(xc + yb + zb) % NUMPTS];
   p021 = pts[(xa + yc + zb) % NUMPTS];
   p121 = pts[(xb + yc + zb) % NUMPTS];
   p221 = pts[(xc + yc + zb) % NUMPTS];
   p002 = pts[(xa + ya + zc) % NUMPTS];
   p102 = pts[(xb + ya + zc) % NUMPTS];
   p202 = pts[(xc + ya + zc) % NUMPTS];
   p012 = pts[(xa + yb + zc) % NUMPTS];
   p112 = pts[(xb + yb + zc) % NUMPTS];
   p212 = pts[(xc + yb + zc) % NUMPTS];
   p022 = pts[(xa + yc + zc) % NUMPTS];
   p122 = pts[(xb + yc + zc) % NUMPTS];
   p222 = pts[(xc + yc + zc) % NUMPTS];

   xf = p.x - xi;
   x1 = xf * xf;
   x2 = 0.5 * x1;
   x1 = 0.5 + xf - x1;
   x0 = 0.5 - xf + x2;

   yf = p.y - yi;
   y1 = yf * yf;
   y2 = 0.5 * y1;
   y1 = 0.5 + yf - y1;
   y0 = 0.5 - yf + y2;

   zf = p.z - zi;
   z1 = zf * zf;
   z2 = 0.5 * z1;
   z1 = 0.5 + zf - z1;
   z0 = 0.5 - zf + z2;

   /*
    * This expression is split up since some compilers
    * chokes on expressions of this size.
    */
   tmp =  z0 * (y0 * (x0 * p000 + x1 * p100 + x2 * p200) +
                y1 * (x0 * p010 + x1 * p110 + x2 * p210) +
                y2 * (x0 * p020 + x1 * p120 + x2 * p220));
   tmp += z1 * (y0 * (x0 * p001 + x1 * p101 + x2 * p201) +
                y1 * (x0 * p011 + x1 * p111 + x2 * p211) +
                y2 * (x0 * p021 + x1 * p121 + x2 * p221));
   tmp += z2 * (y0 * (x0 * p002 + x1 * p102 + x2 * p202) +
                y1 * (x0 * p012 + x1 * p112 + x2 * p212) +
                y2 * (x0 * p022 + x1 * p122 + x2 * p222));

   return tmp;
}



Vector Dnoise(p)
Vector *p;
{
   Vector v;
   int xi, yi, zi;
   int xa, xb, xc, ya, yb, yc, za, zb, zc;
   double xf, yf, zf;
   double x2, x1, x0, y2, y1, y0, z2, z1, z0;
   double xd2, xd1, xd0, yd2, yd1, yd0, zd2, zd1, zd0;
   double p000, p100, p200, p010, p110, p210, p020, p120, p220;
   double p001, p101, p201, p011, p111, p211, p021, p121, p221;
   double p002, p102, p202, p012, p112, p212, p022, p122, p222;

   xi = floor(p->x);
   xa = floor(P1 * (xi * phi - floor(xi * phi)));
   xb = floor(P1 * ((xi + 1) * phi - floor((xi + 1) * phi)));
   xc = floor(P1 * ((xi + 2) * phi - floor((xi + 2) * phi)));

   yi = floor(p->y);
   ya = floor(P2 * (yi * phi - floor(yi * phi)));
   yb = floor(P2 * ((yi + 1) * phi - floor((yi + 1) * phi)));
   yc = floor(P2 * ((yi + 2) * phi - floor((yi + 2) * phi)));

   zi = floor(p->z);
   za = floor(P3 * (zi * phi - floor(zi * phi)));
   zb = floor(P3 * ((zi + 1) * phi - floor((zi + 1) * phi)));
   zc = floor(P3 * ((zi + 2) * phi - floor((zi + 2) * phi)));

   p000 = pts[(xa + ya + za) % NUMPTS];
   p100 = pts[(xb + ya + za) % NUMPTS];
   p200 = pts[(xc + ya + za) % NUMPTS];
   p010 = pts[(xa + yb + za) % NUMPTS];
   p110 = pts[(xb + yb + za) % NUMPTS];
   p210 = pts[(xc + yb + za) % NUMPTS];
   p020 = pts[(xa + yc + za) % NUMPTS];
   p120 = pts[(xb + yc + za) % NUMPTS];
   p220 = pts[(xc + yc + za) % NUMPTS];
   p001 = pts[(xa + ya + zb) % NUMPTS];
   p101 = pts[(xb + ya + zb) % NUMPTS];
   p201 = pts[(xc + ya + zb) % NUMPTS];
   p011 = pts[(xa + yb + zb) % NUMPTS];
   p111 = pts[(xb + yb + zb) % NUMPTS];
   p211 = pts[(xc + yb + zb) % NUMPTS];
   p021 = pts[(xa + yc + zb) % NUMPTS];
   p121 = pts[(xb + yc + zb) % NUMPTS];
   p221 = pts[(xc + yc + zb) % NUMPTS];
   p002 = pts[(xa + ya + zc) % NUMPTS];
   p102 = pts[(xb + ya + zc) % NUMPTS];
   p202 = pts[(xc + ya + zc) % NUMPTS];
   p012 = pts[(xa + yb + zc) % NUMPTS];
   p112 = pts[(xb + yb + zc) % NUMPTS];
   p212 = pts[(xc + yb + zc) % NUMPTS];
   p022 = pts[(xa + yc + zc) % NUMPTS];
   p122 = pts[(xb + yc + zc) % NUMPTS];
   p222 = pts[(xc + yc + zc) % NUMPTS];

   xf = p->x - xi;
   x1 = xf * xf;
   x2 = 0.5 * x1;
   x1 = 0.5 + xf - x1;
   x0 = 0.5 - xf + x2;
   xd2 = xf;
   xd1 = 1.0 - xf - xf;
   xd0 = xf - 1.0;

   yf = p->y - yi;
   y1 = yf * yf;
   y2 = 0.5 * y1;
   y1 = 0.5 + yf - y1;
   y0 = 0.5 - yf + y2;
   yd2 = yf;
   yd1 = 1.0 - yf - yf;
   yd0 = yf - 1.0;

   zf = p->z - zi;
   z1 = zf * zf;
   z2 = 0.5 * z1;
   z1 = 0.5 + zf - z1;
   z0 = 0.5 - zf + z2;
   zd2 = zf;
   zd1 = 1.0 - zf - zf;
   zd0 = zf - 1.0;

   /*
    * This expressions are split up since some compilers
    * chokes on expressions of this size.
    */
   v.x        = z0 * (y0 * (xd0 * p000 + xd1 * p100 + xd2 * p200) +
                      y1 * (xd0 * p010 + xd1 * p110 + xd2 * p210) +
                      y2 * (xd0 * p020 + xd1 * p120 + xd2 * p220));
   v.x       += z1 * (y0 * (xd0 * p001 + xd1 * p101 + xd2 * p201) +
                      y1 * (xd0 * p011 + xd1 * p111 + xd2 * p211) +
                      y2 * (xd0 * p021 + xd1 * p121 + xd2 * p221));
   v.x       += z2 * (y0 * (xd0 * p002 + xd1 * p102 + xd2 * p202) +
                      y1 * (xd0 * p012 + xd1 * p112 + xd2 * p212) +
                      y2 * (xd0 * p022 + xd1 * p122 + xd2 * p222));
                                  
   v.y        = z0 * (yd0 * (x0 * p000 + x1 * p100 + x2 * p200) +
                      yd1 * (x0 * p010 + x1 * p110 + x2 * p210) +
                      yd2 * (x0 * p020 + x1 * p120 + x2 * p220));
   v.y       += z1 * (yd0 * (x0 * p001 + x1 * p101 + x2 * p201) +
                      yd1 * (x0 * p011 + x1 * p111 + x2 * p211) +
                      yd2 * (x0 * p021 + x1 * p121 + x2 * p221));
   v.y       += z2 * (yd0 * (x0 * p002 + x1 * p102 + x2 * p202) +
                      yd1 * (x0 * p012 + x1 * p112 + x2 * p212) +
                      yd2 * (x0 * p022 + x1 * p122 + x2 * p222));
                                  
   v.z        = zd0 * (y0 * (x0 * p000 + x1 * p100 + x2 * p200) +
                       y1 * (x0 * p010 + x1 * p110 + x2 * p210) +
                       y2 * (x0 * p020 + x1 * p120 + x2 * p220));
   v.z       += zd1 * (y0 * (x0 * p001 + x1 * p101 + x2 * p201) +
                       y1 * (x0 * p011 + x1 * p111 + x2 * p211) +
                       y2 * (x0 * p021 + x1 * p121 + x2 * p221));
   v.z       += zd2 * (y0 * (x0 * p002 + x1 * p102 + x2 * p202) +
                       y1 * (x0 * p012 + x1 * p112 + x2 * p212) +
                       y2 * (x0 * p022 + x1 * p122 + x2 * p222));

   return v;
}



double turbulence(p, octaves)
    Vector *p;
    int     octaves;
{
    Vector tmp;
    double scale = 1.0;
    double t     = 0.0;

    while (octaves--) {
        tmp.x = p->x * scale;
        tmp.y = p->y * scale;
        tmp.z = p->z * scale;
        t += (noise(&tmp) / scale);
        scale *= 2.0;
    }
    return t;
}
