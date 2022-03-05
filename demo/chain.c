#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>



#define SMALLRES 15
#define BIGRES   39

extern char *optarg;

main(argc, argv)
    int    argc;
    char **argv;
{
    Object  *torus;
    Object  *torus_pair;
    Object  *chain;
    FILE    *fp ;
    Surf_desc surf;

    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "chain.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "chain.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "chain.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "chain.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "chain.pbm";
            break;

          case 's':
            size = atoi(optarg);
            break;
        }
    }

    sipp_init();

    lightsource_create(1.0, 1.0, 1.0, 0.9, 0.9, 0.9, LIGHT_DIRECTION);
    lightsource_create(-1.0, -1.0, 0.5, 0.4, 0.4, 0.4, LIGHT_DIRECTION);

    surf.ambient = 0.5;
    surf.specular = 0.6;
    surf.c3 = 0.2;
    surf.color.red = 0.8;
    surf.color.grn = 0.6;
    surf.color.blu = 0.3;
    surf.opacity.red = 1.0;
    surf.opacity.grn = 1.0;
    surf.opacity.blu = 1.0;
    
    torus = sipp_torus(1.0, 0.23, BIGRES, SMALLRES, &surf, basic_shader, 
                       WORLD);
    torus_pair = object_create();
    object_add_subobj(torus_pair, torus);
    torus = object_instance(torus);
    object_move(torus, 0.0, -1.375, 0.0);
    object_rot_y(torus, M_PI / 2.0);
    object_add_subobj(torus_pair, torus);
    
    chain = object_create();
    object_move(torus_pair, -1.375, 1.375, 0.0);
    object_add_subobj(chain, torus_pair);
    torus_pair = object_instance(torus_pair);
    object_rot_z(torus_pair, M_PI / 2.0);
    object_move(torus_pair, -1.375, -1.375, 0.0);
    object_add_subobj(chain, torus_pair);
    torus_pair = object_instance(torus_pair);
    object_rot_z(torus_pair, M_PI);
    object_move(torus_pair, 1.375, -1.375, 0.0);
    object_add_subobj(chain, torus_pair);
    torus_pair = object_instance(torus_pair);
    object_rot_z(torus_pair, 3.0 * M_PI / 2.0);
    object_move(torus_pair, 1.375, 1.375, 0.0);
    object_add_subobj(chain, torus_pair);
    
    object_add_subobj(sipp_world, chain);

    camera_params(sipp_camera, 5.0, -2.0, 15.0,  0.5, 0.0, 0.0, 
                  0.0, 0.0, 1.0,  0.25);

    printf("Rendering, wait...");
    fflush(stdout);

    fp = fopen(imfile_name, "w");
    render_image_file(size, size, fp, mode, 2);
    printf("Done.\n");

    exit(0);
}
