#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <shaders.h>
#include <primitives.h>


Wood_desc wood_surf = {
    0.5, 
    0.0, 
    0.99, 
    10.0, 
    {0.770,  0.568,  0.405}, 
    {0.468,  0.296,  0.156},
    {1.0, 1.0, 1.0}
};


extern char *optarg;

main(argc, argv)
    int    argc;
    char **argv;
{
    FILE    *fp ;

    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "wood.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "wood.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "wood.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "wood.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "wood.pbm";
            break;

          case 's':
            size = atoi(optarg);
            break;
        }
    }

    sipp_init();

    lightsource_create(1.0, 1.0, 1.0, 0.9, 0.9, 0.9, LIGHT_DIRECTION);

    object_add_subobj(sipp_world, sipp_block(4.0, 3.0, 3.0, &wood_surf,
                                             wood_shader, WORLD));

    camera_params(sipp_camera, 10.0, 10.0, 20.0,  0.0, 0.0, 0.0,  
                  0.0, 1.0, 0.0,  0.125);

    printf("Rendering, wait...");
    fflush(stdout);

    fp = fopen(imfile_name, "w");
    render_image_file(size, size, fp, mode, 3);
    printf("Done.\n");

    exit(0);
}
