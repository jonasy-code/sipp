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
 ** sipp.h - Public inteface to the sipp rendering library.
 **/


#ifndef _SIPP_H
#define _SIPP_H

/*
 * SIPP is written in standard C (C99 or later).  The macros below are
 * kept so that user code written against older releases keeps compiling.
 */
#undef  _ANSI_ARGS_
#define _ANSI_ARGS_(x)  x
#define _USING_PROTOTYPES_ 1
#undef  CONST
#define CONST const

#undef EXTERN
#ifdef __cplusplus
#   define EXTERN extern "C"
#else
#   define EXTERN extern
#endif

#include <stdio.h>
#include <stdlib.h>
#include <geometric.h>

/*
 * Enable experimantal ffd code.
 */
#define FFD 1

#ifndef M_PI
#define M_PI 3.1415926535897932384626
#endif

/*
 * bool, true and false are keywords from C23 on; before that they come
 * from <stdbool.h>.  TRUE and FALSE are kept for existing code.
 */
#if !defined(__cplusplus) && \
    !(defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
#include <stdbool.h>
#endif
#ifndef FALSE
#define FALSE  0
#define TRUE   1
#endif 

/*
 * Customize for those that don't have memcpy() and friends, but
 * have bcopy() instead.
 */

#ifdef NOMEMCPY
#define memcpy(to, from, n) bcopy((from), (to), (n))
#endif


/*
 * The macro RANDOM() should return a random number
 * in the range [-1, 1].  drand48() is declared in <stdlib.h>.
 */
#define RANDOM()  (2.0 * drand48() - 1.0)


/*
 * Modes for rendering
 */
#define PHONG      0
#define GOURAUD    1
#define FLAT       2
#define LINE       3


/*
 * Direction for rendering
 */
#define TOP_TO_BOTTOM  FALSE
#define BOTTOM_TO_TOP  TRUE


/*
 * Field definition.
 */
#define EVEN   0
#define ODD    1
#define BOTH   2


/*
 * Types of lightsources.
 */
#define LIGHT_DIRECTION    0
#define LIGHT_POINT        1

/*
 * Types of spotlights (actually lightsource types too).
 */
#define SPOT_SHARP   2
#define SPOT_SOFT    3




/*
 * FFD function interface.
 */
#ifdef FFD
typedef void FFD_func (void    *ffd_data,
                                   Vector  *pos,
                                   Vector  *texture,
                                   Vector  *ffd_pos,
                                   Vector  *ffd_texture);
#endif

/*
 * Update function interface.
 */
typedef void Update_func (void  *client_data);


/*
 * Colors are handled as an rgb-triple
 * with values between 0 and 1.
 */
typedef struct {
    double   red;
    double   grn;
    double   blu;
} Color;


/*
 * Interface to shader functions.  SURFACE is the surface description the
 * shader was installed with (see surface_create()); each shader casts it
 * to its own descriptor type.
 */
struct lightsource_t;
typedef void Shader(Vector               *pos,
                    Vector               *normal,
                    Vector               *texture,
                    Vector               *view_vec,
                    struct lightsource_t *lights,
                    void                 *surface,
                    Color                *color,
                    Color                *opacity);

/*
 * Interface to the user supplied output functions of render_image_func()
 * and render_field_func().  A Pixel_func receives one pixel of a shaded
 * image.  In LINE mode a Line_func is used instead, receiving one line
 * segment in image coordinates; pass it as (Pixel_func *) to the
 * rendering function.
 */
typedef void Pixel_func(void          *data,
                        int            x,
                        int            y,
                        unsigned char  red,
                        unsigned char  grn,
                        unsigned char  blu);

typedef void Line_func(void *data,
                       int   x1,
                       int   y1,
                       int   x2,
                       int   y2);


/*
 * Virtual camera definition
 */
typedef struct {
    Vector position;     /* camera position */
    Vector lookat;       /* point to look at */
    Vector up;           /* Up direction in the view */ 
    double focal_ratio;
} Camera;


/*
 * Surface description used by the basic shader. This shader
 * does simple shading of surfaces of a single color.
 */
typedef struct {
    double  ambient;       /* Fraction of color visible in ambient light */
    double  specular;      /* Fraction of colour specularly reflected */
    double  c3;            /* "Shinyness" 0 = shiny,  1 = dull */
    Color   color;         /* Colour of the surface */
    Color   opacity;       /* Opacity of the surface */
} Surf_desc;


#ifdef FFD
/*
 * Structure for temporary vertex data after ffd.
 */
typedef struct {
    Vector  pos;
    Vector  texture;
} FFD_Vertex;
#endif 

/*
 * Structure storing the vertices in surfaces. The vertices of a surface
 * are kept in a singly linked list (through NEXT).  While a surface is
 * being built, vertices are also entered into a spatial hash table so
 * that shared vertices can be found quickly; HNEXT links the vertices
 * within one hash bucket and has no meaning once the surface is created
 * (objects.c reuses it as scratch space when copying a surface).
 */
typedef struct vertex_t {
    Vector            pos;    /* vertex position */
    Vector            normal;    /* average normal at vertex */
    Vector            texture;    /* texture parameters (if any) */
    bool              fixed_normal; /* Normal is fixed (supplied by user) */
#ifdef FFD
    FFD_Vertex       *ffd_vertex;
#endif 
    struct vertex_t  *next;   /* next vertex in the surface */
    struct vertex_t  *hnext;  /* next vertex in the same hash bucket */
} Vertex;


/*
 * Polygon definition. A polygon is defined by a list of
 * references to its vertices (counterclockwize order).
 */
typedef struct polygon_t {
    int         nvertices;
    Vertex    **vertex;
    bool        backface;   /* polygon is backfacing (used at rendering) */
    struct polygon_t *next;
} Polygon;


/*
 * Optional header for a surface descriptor associated with a shader. This
 * header is supplied for dynamically allocated descriptors that are to
 * be released when no longer in use by any surfaces.  When the reference
 * count goes to zero, the free_func stored in the header is called with a
 * pointer to the header stored.  The surface descriptor must immediately
 * follow it.  When the surface is defined, a global flag set by
 * sipp_surface_desc_headers is check so see if the header is  before the
 * descriptor.
 */
typedef struct {
    int         ref_count;       /* # of references surface description */
    void      (*free_func)(void *); /* called with the header when no more refs */
    void       *client_data;     /* arbitrary data for free_func to use  */
} Surf_desc_hdr;

/*
 * Macro to allocate any structure with the above header.
 */
#define SIPP_SURF_HDR_ALLOC(type) ((Surf_desc_hdr *) \
                                   smalloc(sizeof(Surf_desc_hdr) \
                                           + sizeof(type)))
/*
 * Macro to extract a pointer of any type to the data after
 * a surface description header.
 */
#define SIPP_SURFP_HDR(type, hdr) ((type *)(((char *)hdr) \
                                            + sizeof(Surf_desc_hdr)))

/*
 * Surface definition. Each surface consists of a vertex tree, 
 * a polygon list, possibly a surface description header, a pointer to a
 *  surface description and a pointer to a shader function.
 */
typedef struct surface_t {
    Vertex           *vertices;          /* list of vertices */
    Polygon          *polygons;          /* polygon list */
    Surf_desc_hdr    *surf_desc_hdr;     /* header for surface if not NULL */
    void             *surface;           /* surface description */
    Shader           *shader;            /* shader function */
/*    Vector            max, min;          / * Bounding box (Future use) */
#ifdef FFD
    Surf_desc_hdr    *ffd_desc_hdr;      /* header for ffd_data if not NULL */
    void             *ffd_data;
    FFD_func         *ffd_func;
#endif 
    int               ref_count;         /* # of references to this surface */
} Surface;


/*
 * Object definition. Object consists of one or more
 * surfaces and/or one or more subojects. Each object
 * has its own transformation matrix that affects itself
 * and all its subobjects.
 */
typedef struct object_t {
    Surface         **surfaces;       /* Table of surfaces */
    int               num_surfaces;   /* Number of surfaces */
    int               surfaces_size;  /* Size of surfaces table */
    struct object_t **sub_objs;       /* Table of subobjects */
    int               num_sub_objs;   /* Number of subobjects */
    int               sub_objs_size;  /* Size of subobjects table */
    Transf_mat        transf;         /* Transformation matrix */
    int               ref_count;      /* # of references to this object */
} Object;


/*
 * Information needed in a lightsource to generate
 * shadows.
 */
typedef struct {
    Transf_mat  matrix;
    double      fov_factor;
    double      bias;
    bool        active;
    float      *d_map;
} Shadow_info;


/*
 * Public part of lightsource definition.
 * Used for both normal lightsources and spotlights.
 */
typedef struct lightsource_t {
    Color                 color;      /* Color of the lightsource */
    bool                  active;     /* Is the light on? */
    int                   type;       /* Type of lightsource */
    void                 *info;       /* Type dependent info */
    Shadow_info           shadow;     /* Shadow information */
    struct lightsource_t *next;       /* next lightsource in the list */
} Lightsource;


extern char  * SIPP_VERSION;


/*
 * The world that is rendered. Defined in objects.c
 */
extern Object  *sipp_world;


/*
 * The internal (default) camera.
 */
extern Camera  *sipp_camera;


/*
 * This defines all public functions implemented in sipp.
 */

/* Global initialization and configuration functions. */
EXTERN void
sipp_init(void);

EXTERN void
sipp_show_backfaces(bool flag_);

EXTERN void
sipp_render_direction(bool direction);

EXTERN void
sipp_background(double red,
                             double grn,
                             double blu);

EXTERN void
sipp_set_update_callback(Update_func *func,
                                      void        *client_data,
                                      int          period);

EXTERN void
sipp_render_threads(int n);

EXTERN void
sipp_shading_per_pixel(bool flag);

EXTERN void
sipp_shadows(bool flag,
                          int  size);

EXTERN bool
sipp_user_refcount(bool flag);

EXTERN bool
sipp_surface_desc_headers(bool flag);

#ifdef FFD
EXTERN bool
sipp_ffd_desc_headers(bool flag);
#endif 

/* Functions for handling surfaces and objects. */

EXTERN void
vertex_push(double  x,
                         double  y,
                         double  z);

EXTERN void
vertex_tx_push(double  x,
                            double  y,
                            double  z,
                            double  u,
                            double  v,
                            double  w);

EXTERN void
vertex_n_push(double  x,
                           double  y,
                           double  z,
                           double nx,
                           double ny,
                           double nz);

EXTERN void
vertex_tx_n_push(double   x,
                              double   y,
                              double   z,
                              double   u,
                              double   v,
                              double   w,
                              double  nx,
                              double  ny,
                              double  nz);

EXTERN void
polygon_push(void);

EXTERN Surface
*surface_create (void   *surf_desc,
                             Shader *shader);

EXTERN void
surface_unref(Surface *surface);

EXTERN void
surface_desc_unref(Surf_desc_hdr *surf_desc_hdr);

EXTERN Surface *
surface_basic_create(double   ambient,
                                  double   red,
                                  double   grn,
                                  double   blu,
                                  double   specular,
                                  double   c3,
                                  double   opred,
                                  double   opgrn,
                                  double   opblu);

EXTERN void
surface_set_shader(Surface *surface,
                                void    *surf_desc,
                                Shader  *shader);

#ifdef FFD
EXTERN void
surface_set_ffd(Surface  *surface,
                             FFD_func *ffd_func,
                             void     *ffd_data);
#endif 

EXTERN void
surface_basic_shader(Surface   *surface,
                                  double     ambient,
                                  double     red,
                                  double     grn,
                                  double     blu,
                                  double     specular,
                                  double     c3,
                                  double     opred,
                                  double     opgrn,
                                  double     opblu);

EXTERN Object *
object_create(void);

EXTERN Object *
object_instance(Object *object);

EXTERN Object *
object_dup(Object *object);

EXTERN Object *
object_deep_dup(Object *object);

EXTERN void
object_unref(Object *object);

EXTERN void
object_add_surface(Object  *object,
                                Surface *surface);

EXTERN bool
object_sub_surface(Object   *object,
                                Surface  *surface);

EXTERN void
object_add_subobj(Object *object,
                               Object *subobj);

EXTERN bool
object_sub_subobj(Object *object,
                               Object *subobj);

/* Functions for handling transforming objects. */

EXTERN void
object_set_transf(Object     *obj,
                               Transf_mat *matrix);

EXTERN Transf_mat *
object_get_transf(Object     *obj,
                               Transf_mat *matrix);

EXTERN void
object_clear_transf(Object *obj);

EXTERN void
object_transform(Object     *obj,
                              Transf_mat *matrix);

EXTERN void
object_rot_x(Object *obj,
                          double  ang);

EXTERN void
object_rot_y(Object *obj,
                          double  ang);

EXTERN void
object_rot_z(Object *obj,
                          double  ang);

EXTERN void
object_rot(Object *obj,
                        Vector *point,
                        Vector *vec,
                        double  ang);

EXTERN void
object_scale(Object *obj,
                          double  xscale, 
                          double  yscale,
                          double  zscale);

EXTERN void
object_move(Object *obj,
                         double  dx,
                         double  dy,
                         double  dz);

/* Functions for handling lightsources and spotlights. */

EXTERN Lightsource *
lightsource_create(double  x,
                                double  y,
                                double  z,
                                double  red,
                                double  grn,
                                double  blu,
                                int     type);

EXTERN Lightsource *
spotlight_create(double  x,
                              double  y,
                              double  z,
                              double  to_x,
                              double  to_y,
                              double  to_z,
                              double  fov,
                              double  red,
                              double  grn,
                              double  blu,
                              int     type,
                              bool    shadows);

EXTERN void
light_destruct(Lightsource   *light);

EXTERN void
lightsource_put(Lightsource *lp,
                             double       x,
                             double       y, 
                             double       z);

EXTERN void
spotlight_pos(Lightsource *lp,
                           double       x,
                           double       y,
                           double       z);

EXTERN void
spotlight_at(Lightsource *lp,
                          double       x,
                          double       y,
                          double       z);

EXTERN void
spotlight_opening(Lightsource *lp,
                               double       fov);

EXTERN void
spotlight_shadows(Lightsource *lp,
                               bool         flag);

EXTERN void
light_color(Lightsource *lp,
                         double       red, 
                         double       grn,
                         double       blu);

EXTERN void
light_active(Lightsource *lp,
                          bool         flag);

EXTERN double
light_eval(Lightsource *lp,
                        Vector      *pos,
                        Vector      *vec);

/* Functions for handling the viewpoint and virtual cameras. */

EXTERN Camera *
camera_create(void);

EXTERN void
camera_destruct(Camera *cp);

EXTERN void
camera_position(Camera *cp,
                             double  x,
                             double  y,
                             double  z);

EXTERN void
camera_look_at(Camera *cp,
                            double  x,
                            double  y,
                            double  z);

EXTERN void
camera_up(Camera *cp,
                       double  x,
                       double  y,
                       double   z);

EXTERN void
camera_focal(Camera *cp,
                          double  focal);

EXTERN void
camera_params(Camera *cp,
                           double  x0,
                           double  y0,
                           double  z0,
                           double  x,
                           double  y,
                           double  z,
                           double  ux,
                           double  uy,
                           double  uz,
                           double  ratio);

EXTERN void
camera_use(Camera *cp);

/* Functions to render an image. */

EXTERN void
render_image_file(int   xres, 
                               int   yres,
                               FILE *im_file,
                               int   render_mode,
                               int   oversampling);

EXTERN void
render_image_func(int      xres, 
                               int      yres,
                               Pixel_func *pixel_func,
                               void    *data,
                               int      render_mode,
                               int      oversampling);

EXTERN void
render_field_file(int   xres,
                               int   yres,
                               FILE *im_file,
                               int   render_mode,
                               int   oversampling,
                               int   field);

EXTERN void
render_field_func(int      xres, 
                               int      yres,
                               Pixel_func *pixel_func,
                               void    *data,
                               int      render_mode,
                               int      oversampling,
                               int      field);

EXTERN void
sipp_render_terminate(void);

EXTERN void
shadowmaps_create(int size);

EXTERN void
shadowmaps_destruct(void);

/* The basic shader. */
EXTERN void
basic_shader(Vector      *pos,
                          Vector      *normal,
                          Vector      *texture,
                          Vector      *view_vec,
                          Lightsource *lights,
                          void        *sd,        /* Surf_desc * */
                          Color       *color,
                          Color       *opacity);

/*
 * The following functions & macros are provided for backward compatibility.
 * We plan to remove them from future releases though,
 * so we don't encourage use of them.
 */
EXTERN void
object_delete(Object *o);

#define object_install(obj)    object_add_subobj(sipp_world, obj);
#define object_uninstall(obj)  object_sub_subobj(sipp_world, obj);
#define view_from(x, y, z)     camera_position(sipp_camera, x, y, z)
#define view_at(x, y, z)       camera_look_at(sipp_camera, x, y, z)
#define view_up(x, y, z)       camera_up(sipp_camera, x, y, z)
#define view_focal(fr)         camera_focal(sipp_camera, fr)
#define viewpoint(x, y, z, x2, y2, z2, ux, uy, uz, fr)\
    camera_params(sipp_camera, x, y, z, x2, y2, z2, ux, uy, uz, fr)
#define lightsource_push(x, y, z, i) \
    lightsource_create(x, y, z, i, i, i, LIGHT_DIRECTION)
#define render_image_pixmap(w, h, p, f, m, o) \
    render_image_func(w, h, f, p, m, o)

#endif /* _SIPP_H */
