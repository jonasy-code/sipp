#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>



#define RESOLUTION 15

extern char *optarg;

main(argc, argv)
    int    argc;
    char **argv;
{
    FILE    *fp ;
    Surf_desc surf;

    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "ellipsoid.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "ellipsoid.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "ellipsoid.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "ellipsoid.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "ellipsoid.pbm";
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
    surf.color.red = 0.6;
    surf.color.grn = 0.3;
    surf.color.blu = 0.5;
    surf.opacity.red = 1.0;
    surf.opacity.grn = 1.0;
    surf.opacity.blu = 1.0;
    
    object_add_subobj(sipp_world, sipp_ellipsoid(1.0, 2.0, 3.0, RESOLUTION,
                                  &surf, basic_shader, WORLD)); 

    camera_params(sipp_camera, 10.0, 0.0, 0.0,  0.0, 0.0, 0.0,  
                  0.0, 0.0, 1.0,  0.4);

    printf("Rendering, wait...");
    fflush(stdout);

    fp = fopen(imfile_name, "w");
    render_image_file(size, size, fp, mode, 2);
    printf("Done.\n");

    exit(0);
}
