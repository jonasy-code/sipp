#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>
#include <shaders.h>



#define SUBDIVS  50

extern char *optarg;

main(argc, argv)
    int    argc;
    char **argv;
{
    Surf_desc   planet_surface;
    Object     *planet;
    FILE       *outfile;

    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "planet.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "planet.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "planet.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "planet.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "planet.pbm";
            break;

          case 's':
            size = atoi(optarg);
            break;
        }
    }
    
    planet_surface.ambient = 0.4;
    planet_surface.specular = 0.0;
    planet_surface.c3 = 0.5;
    planet_surface.color.red = 1.0;
    planet_surface.color.grn = 0.0;
    planet_surface.color.blu = 0.0;
    planet_surface.opacity.red = 1.0;
    planet_surface.opacity.grn = 1.0;
    planet_surface.opacity.blu = 1.0;

    sipp_init();

    lightsource_create(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, LIGHT_DIRECTION);

    object_add_subobj(sipp_world, sipp_sphere(1.0, SUBDIVS, &planet_surface, 
                                              planet_shader, WORLD)); 
    object_rot_z(sipp_world, -1.2);
    object_rot_x(sipp_world, 0.2);

    camera_params(sipp_camera, 0.0, 2.0, 0.0,  0.0, 0.0, 0.0,  
                  0.0, 0.0, 1.0,  0.75);

    printf("Rendering, wait...");
    fflush(stdout);

    outfile = fopen(imfile_name, "w");
    render_image_file(size, size, outfile, mode, 3);
    printf("Done.\n");
    
    exit(0);
}
