#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <geometric.h>
#include <noise.h>


/* A reasonably nice brown color */
static Color  land = {0.28125, 0.1875, 0.09375};

/* Oceans are usually blue */
static Color  sea = {0.0, 0.0, 1.0};

/* And Clouds are white */
static Color  cloud = {1.0, 1.0, 1.0};


/* 
 * This was designed to work with a unit sphere.  
 * 
 * Thanks to Jon Buller       jonb@vector.dallas.tx.us
 */
double
turb(size, scale_factor, loc)
    int size;
    double scale_factor;
    Vector loc;
{
    double cur_scale, result;
    int cur;

    result = noise(&loc);
    cur_scale = 1.0;

    cur = 1;
    while (cur < size) {
        cur <<= 1;
        cur_scale = cur_scale * scale_factor;
        loc.x *= 2.0;
        loc.y *= 2.0;
        loc.z *= 2.0;
        result += noise(&loc) * cur_scale;
    }
    return result;
}



extern bool noise_ready;

void
planet_shader(pos, normal, texture, view_vec, lights, sd, color, opacity)
    Vector      *pos;
    Vector      *normal;
    Vector      *texture;
    Vector      *view_vec;
    Lightsource *lights;
    Surf_desc   *sd;
    Color       *color;
    Color       *opacity;
{
    Vector  tmp;
    double  amt;

    if (!noise_ready) {
        noise_init();
    }

    VecCopy(tmp, *texture);

    if (turb(430, 0.7, tmp) > 0.15)
        sd->color = land;
    else 
        sd->color = sea;

    VecScalMul(tmp, 12.0, tmp)

    amt = turb(18, 0.6, tmp);
    if (amt > -0.25) {
        amt += 0.25;
        sd->color.red += amt * (cloud.red - sd->color.red);
        sd->color.grn += amt * (cloud.grn - sd->color.grn);
        sd->color.blu += amt * (cloud.blu - sd->color.blu);
    }

    basic_shader(pos, normal, texture, view_vec, lights, sd, 
                 color, opacity);
}
