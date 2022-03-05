/*
 * Demo of the strauss shader. Four spheres are rendered, 
 * all with the same base color, but different smoothness
 * and metalness.
 */

#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <shaders.h>
#include <primitives.h>


/*
 * Surface description of the spheres.
 */
Strauss_desc non_metal_dull = {
    0.4,                        /* Ambient */
    0.2,                        /* Smoothness */
    0.1,                        /* Metalness */
    {0.6, 0.33, 0.27},          /* Color */
    {1.0, 1.0, 1.0}             /* Opacity */
};

Strauss_desc non_metal_shiny = {
    0.4, 
    0.7, 
    0.1, 
    {0.6, 0.33, 0.27}, 
    {1.0, 1.0, 1.0}
};

Strauss_desc metal_dull = {
    0.4, 
    0.2, 
    0.9, 
    {0.6, 0.33, 0.27}, 
    {1.0, 1.0, 1.0}
};

Strauss_desc metal_shiny = {
    0.4, 
    0.7, 
    0.9, 
    {0.6, 0.33, 0.27}, 
    {1.0, 1.0, 1.0}
};


/*
 * White surface as background.
 */
Surf_desc bg_surf = {
    0.4,
    0.0,
    0.99,
    {0.9804,  0.9216,  0.8431},  /* Antique white */
    {1.0, 1.0, 1.0}
};



#define RESOLUTION 30

extern char *optarg;

main(argc, argv)
    int    argc;
    char **argv;
{
    Object *nmd;     /* Non metallic, dull sphere */
    Object *nms;     /* Non metallic, shiny sphere */
    Object *md;      /* Metallic, dull sphere */
    Object *ms;      /* Metallic, shiny sphere */
    Object *bg;      /* Background */
    FILE   *fp;

    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "strauss.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "strauss.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "strauss.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "strauss.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "strauss.pbm";
            break;

          case 's':
            size = atoi(optarg);
            break;
        }
    }

    sipp_init();

    lightsource_create(-1.0, -1.0, 1.0, 1.0, 1.0, 1.0, LIGHT_DIRECTION);
    lightsource_create(0.0, -1.0, 2.0, 0.6, 0.6, 0.6, LIGHT_DIRECTION);

    /*
     * Non metal, dull. Upper left.
     */
    nmd = sipp_sphere(1.0, RESOLUTION, &non_metal_dull, strauss_shader, WORLD);
    object_move(nmd, -1.1, 0.0, 1.1);
    object_add_subobj(sipp_world, nmd); 

    /*
     * Non metal, shiny. Upper right.
     */
    nms = sipp_sphere(1.0, RESOLUTION, &non_metal_shiny, strauss_shader,
                      WORLD); 
    object_move(nms, 1.1, 0.0, 1.1);
    object_add_subobj(sipp_world, nms); 

    /*
     * Metal, dull. Lower left.
     */
    md = sipp_sphere(1.0, RESOLUTION, &metal_dull, strauss_shader, WORLD);
    object_move(md, -1.1, 0.0, -1.1);
    object_add_subobj(sipp_world, md); 
 
    /*
     * Metal, shiny. Lower right.
     */
    ms = sipp_sphere(1.0, RESOLUTION, &metal_shiny, strauss_shader, WORLD);
    object_move(ms, 1.1, 0.0, -1.1);
    object_add_subobj(sipp_world, ms);

    /*
     * Background.
     */
    bg = sipp_block(10.0, 0.5, 10.0, &bg_surf, basic_shader, WORLD);
    object_move(bg, 0.0, 1.5, 0.0);
    object_add_subobj(sipp_world, bg);

    camera_params(sipp_camera, 0.0, -10.0, 0.0,  0.0, 0.0, 0.0,  
                  0.0, 0.0, 1.0,  0.25);

    printf("Rendering, wait...");
    fflush(stdout);

    fp = fopen(imfile_name, "w");
    render_image_file(size, size, fp, mode, 2);
    printf("Done.\n");

    exit(0);
}
