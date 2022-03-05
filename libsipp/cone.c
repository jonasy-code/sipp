/*
 * File:  sipp_cone.c
 * 
 * Create a, possibly truncated, cone. Cylinder is a special case
 * of a truncated cone with the same top and bottom radius.
 *
 * Author:  David Jones
 *          djones@awesome.berkeley.edu
 *
 * Adapted for inclusion into the SIPP package by Jonas Yngvesson
 */

#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>
#include <xalloca.h>


Object *
sipp_cone(radius_bot, radius_top, length, res, surface, shader, texture)
    double    radius_bot;
    double    radius_top;
    double    length;
    int	      res;
    void    * surface;
    Shader  * shader;
    int       texture;
{
    Object  * cone;
    double  * xb;
    double  * yb;
    double  * ub;
    double  * vb;
    double  * xt;
    double  * yt;
    double  * ut;
    double  * vt;
    double    frac;
    double    half_length;
    int       bot_exists;
    int       top_exists;
    int       i;
    bool      old_user_refs;
    
    /*
     * If both top and bottom radii is zero it's a line
     * and can't be rendered.
     */
    if (radius_bot == 0.0 && radius_top == 0.0) {
        return NULL;
    }

    if (radius_bot > 0.0) {
        xb = (double *) alloca((res + 1) * sizeof(double));
        yb = (double *) alloca((res + 1) * sizeof(double));
        ub = (double *) alloca((res + 1) * sizeof(double));
        vb = (double *) alloca((res + 1) * sizeof(double));
        bot_exists = 1;
    } else if (radius_bot == 0.0 ){
        bot_exists = 0;
    } else {
        return NULL;
    }

    if (radius_top > 0.0) {
        xt = (double *) alloca((res + 1) * sizeof(double));
        yt = (double *) alloca((res + 1) * sizeof(double));
        ut = (double *) alloca((res + 1) * sizeof(double));
        vt = (double *) alloca((res + 1) * sizeof(double));
        top_exists = 1;
    } else if (radius_top == 0.0) {
        top_exists = 0;
    } else {
        return NULL;
    }
    
    half_length = length * 0.5;

    /* list of coordinates */
    for (i = 0; i <= res ; ++i) {
        frac = ((double) i) / ((double) res);
        if (bot_exists) {
            xb[i] = radius_bot * cos(frac * 2.0 * M_PI);
            yb[i] = radius_bot * sin(frac * 2.0 * M_PI);
            switch (texture) {
              case NATURAL:
              case CYLINDRICAL:
                ub[i] = frac;
                vb[i] = 0.0;
                break;

              case SPHERICAL:
                ub[i] = frac;
                vb[i] = atan(-half_length / radius_bot) / M_PI + 0.5;
                break;

              case WORLD:
              default:
                ub[i] = xb[i];
                vb[i] = yb[i];
                break;
            }
        }
        if (top_exists) {
            xt[i] = radius_top * cos(frac * 2.0 * M_PI);
            yt[i] = radius_top * sin(frac * 2.0 * M_PI);
            switch (texture) {
              case NATURAL:
              case CYLINDRICAL:
                ut[i] = frac;
                vt[i] = 1.0;
                break;

              case SPHERICAL:
                ut[i] = frac;
                vt[i] = atan(half_length / radius_top) / M_PI + 0.5;
                break;

              case WORLD:
              default:
                ut[i] = xt[i];
                vt[i] = yt[i];
                break;
            }
        }
    }
    
    /* empty object */
    cone = object_create();
    
    old_user_refs = sipp_user_refcount (FALSE);

    /* The bottom surface */
    if (bot_exists) {
        for (i = res; i > 0 ; --i) {
            vertex_tx_push(xb[i], yb[i], -half_length,
                           ub[i], vb[i], -half_length);
            vertex_tx_push(xb[i - 1], yb[i - 1], -half_length,
                           ub[i - 1], vb[i - 1], -half_length);
            switch (texture) {
              case NATURAL:
              case CYLINDRICAL:
                vertex_tx_push(0.0, 0.0, -half_length,
                               ub[i - 1], vb[i - 1], -half_length);
                vertex_tx_push(0.0, 0.0, -half_length,
                               ub[i], vb[1], -half_length);
                break;

              case SPHERICAL:
                vertex_tx_push(0.0, 0.0, -half_length,
                               ub[i - 1], 0.0, -half_length);
                vertex_tx_push(0.0, 0.0, -half_length,
                               ub[i], 0.0, -half_length);
                break;

              case WORLD:
              default:
                vertex_tx_push(0.0, 0.0, -half_length, 
                               0.0, 0.0, -half_length);
                break;
            }
            polygon_push();
        } 
        object_add_surface(cone, surface_create(surface, shader));
    }
    
    /* The top surface */
    if (top_exists) {
        for (i = 0; i < res ; ++i) {
            vertex_tx_push(xt[i], yt[i], half_length,
                           ut[i], vt[i], half_length);
            vertex_tx_push(xt[i + 1], yt[i + 1], half_length,
                           ut[i + 1], vt[i + 1], half_length);
            switch (texture) {
              case NATURAL:
              case CYLINDRICAL:
                vertex_tx_push(0.0, 0.0, half_length,  
                               ut[i + 1], vt[i + 1], half_length);
                vertex_tx_push(0.0, 0.0, half_length,  
                               ut[i], vt[i], half_length);
                break;

              case SPHERICAL:
                vertex_tx_push(0.0, 0.0, half_length,  
                               ut[i + 1], 1.0, half_length);
                vertex_tx_push(0.0, 0.0, half_length,  
                               ut[i], 1.0, half_length);
                break;

              case WORLD:
              default:
                vertex_tx_push(0.0, 0.0, half_length,
                               0.0, 0.0, half_length);
                break;
            }
            polygon_push();
        }
        object_add_surface(cone, surface_create(surface, shader));
    }
    
    /* The side surface */
    for (i = 0; i < res ; ++i) {
        if (top_exists && bot_exists) {
            vertex_tx_push(xb[i], yb[i], -half_length,
                           ub[i], vb[i], -half_length);
            vertex_tx_push(xb[i+1], yb[i+1], -half_length,
                           ub[i+1], vb[i+1], -half_length);
            vertex_tx_push(xt[i+1], yt[i+1], half_length,
                           ut[i+1], vt[i+1], half_length);
            vertex_tx_push(xt[i], yt[i], half_length,
                           ut[i], vt[i], half_length);
        } else if (top_exists) {
            vertex_tx_push(0.0, 0.0, -half_length,
                           0.0, 0.0, -half_length);
            vertex_tx_push(xt[i+1], yt[i+1], half_length,
                           ut[i+1], vt[i+1], half_length);
            vertex_tx_push(xt[i], yt[i], half_length,
                           ut[i], vt[i], half_length);
        } else {
            vertex_tx_push(xb[i], yb[i], -half_length,
                           ub[i], vb[i], -half_length);
            vertex_tx_push(xb[i+1], yb[i+1], -half_length,
                           ub[i+1], vb[i+1], -half_length);
            switch (texture) {
              case NATURAL:
              case CYLINDRICAL:
              case SPHERICAL:
                vertex_tx_push(0.0, 0.0, half_length,
                               0.0, 1.0, half_length);
                break;

              case WORLD:
              default:
                vertex_tx_push(0.0, 0.0, half_length,
                               0.0, 0.0, half_length);
                break;
            }
        }
        polygon_push();
    }
    object_add_surface(cone, surface_create(surface, shader));

    sipp_user_refcount (old_user_refs);
    
    return cone;
}



Object *
sipp_cylinder(radius, length, res, surface, shader, texture)
    double    radius;
    double    length;
    int       res;
    void     *surface;
    Shader   *shader;
    int       texture;
{
    Object   *cylinder;

    cylinder = sipp_cone(radius, radius, length, res, surface, shader,
                         texture); 

    return cylinder;
}
