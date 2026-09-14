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
 ** shaders.h - This include file defines the different shaders availiable
 **             in sipp. Each shader is defined by a structure containing
 **             the necessary parameters to describe how a surface should
 **             be shaded with that particular shader, and the extern
 **             declaration of the shader function itself.
 **/

#ifndef _SHADERS_H
#define _SHADERS_H

#include <sipp.h>

/*
 * Surface description used in phong_shader().
 */
typedef struct {
  double ambient;  /* Fraction of color visible in ambient light */
  double diffuse;  /* Diffuse reflexion factor */
  double specular; /* Specular reflexion factor */
  int spec_exp;    /* Exponent in the specular calculation */
  Color color;     /* Color of the surface */
  Color opacity;   /* Opacity of the surface */
} Phong_desc;

/*
 * Surface description used in strauss_shader().
 */
typedef struct {
  double ambient;    /* Fraction of color visible in ambient light */
  double smoothness; /* Smoothness of the surface [0, 1] */
  double metalness;  /* Metalness of the surface [0, 1] */
  Color color;       /* Base color of the surface */
  Color opacity;     /* Opacity of the surface */
} Strauss_desc;

/*
 * Surface description for the bozo shader.
 */
typedef struct {
  Color *colors;
  int no_of_cols;
  double ambient;
  double specular;
  double c3;
  double scale;  /* Scale the texture by this value */
  Color opacity; /* Opacity of the surface */
} Bozo_desc;

/*
 * Surface description used by the wood shader. This shader
 * creates a solid texture (using noise & turbulence) that
 * simulates wood.
 */
typedef struct {
  double ambient;
  double specular;
  double c3;
  double scale;  /* Scale the wood texture by this value */
  Color base;    /* "Base" color of the surface */
  Color ring;    /* Color of the darker rings */
  Color opacity; /* Opacity of the surface */
} Wood_desc;

/*
 * Surface description used by the marble shader. marble_shader
 * creates a solid texture (using noise & turbulence) that
 * simulates marble.
 */
typedef struct {
  double ambient;
  double specular;
  double c3;
  double scale;  /* Scale the marble texture by this value */
  Color base;    /* "Base" color of the surface */
  Color strip;   /* Color of the "stripes" in the marble */
  Color opacity; /* Opacity of the surface */
} Marble_desc;

/*
 * Surface description used by the granite shader. granite_shader
 * creates a solid texture (using noise) that mixes two colors
 * to simulate granite.
 */
typedef struct {
  double ambient;
  double specular;
  double c3;
  double scale; /* Scale the texture by this value */
  Color col1;   /* The two color components */
  Color col2;
  Color opacity; /* Opacity of the surface */
} Granite_desc;

/*
 * Mask shader. It uses a mask (usually an image) to choose between
 * two other shaders.  When a surface is shaded it calls masker() with
 * the point, which returns the fraction, 0.0 to 1.0, of the shading
 * sample that the mask covers; t_shader is used where the mask is
 * set, f_shader where it is not, and the two are blended in between
 * (a filtered mask, e.g. sipp_texture_lookup() on a one channel
 * texture, gives anti-aliased mask edges).
 */
typedef struct {
  Shader *t_shader; /* Shader to call where the mask is set */
  void *t_surface;  /* Surface description for t_shader */
  Shader *f_shader; /* Shader to call where the mask is not set */
  void *f_surface;  /* Surface description for f_shader */
  void *mask_data;  /* Pointer to data for masking function */
  double (*masker)(void *mask_data, const Shade_point *sp); /* Coverage */
} Mask_desc;

/*
 * Surface description for the bumpy_shader(). This shader
 * fiddles with the surface normals of a surface so the surface
 * looks bumpy.
 */

typedef struct {
  Shader *shader;
  void *surface;
  double scale;
  bool bumpflag;
  bool holeflag;
} Bumpy_desc;

/*
 * Declarations of the actual shading functions.
 */
extern void phong_shader(const Shade_point *sp, Lightsource *lights,
                         void *pd, /* Phong_desc * */
                         Color *color, Color *opacity);

extern void strauss_shader(const Shade_point *sp, Lightsource *lights,
                           void *sd, /* Strauss_desc * */
                           Color *color, Color *opacity);

extern void marble_shader(const Shade_point *sp, Lightsource *lights,
                          void *md, /* Marble_desc * */
                          Color *color, Color *opacity);

extern void granite_shader(const Shade_point *sp, Lightsource *lights,
                           void *gd, /* Granite_desc * */
                           Color *color, Color *opacity);

extern void bozo_shader(const Shade_point *sp, Lightsource *lights,
                        void *bd, /* Bozo_desc * */
                        Color *color, Color *opacity);

extern void mask_shader(const Shade_point *sp, Lightsource *lights,
                        void *md, /* Mask_desc * */
                        Color *color, Color *opacity);

extern void bumpy_shader(const Shade_point *sp, Lightsource *lights,
                         void *bd, /* Bumpy_desc * */
                         Color *color, Color *opacity);

extern void planet_shader(const Shade_point *sp, Lightsource *lights,
                          void *sd, /* Surf_desc * */
                          Color *color, Color *opacity);

extern void wood_shader(const Shade_point *sp, Lightsource *lights,
                        void *wd, /* Wood_desc * */
                        Color *color, Color *opacity);

#endif /* _SHADERS_H */
