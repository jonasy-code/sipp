#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>


/* The XY coordinats for a 3-sided prism. */
Vector   prism3[3] = {
    { 1.0, -1.0, 0.0}, 
    {-1.0,  2.0, 0.0}, 
    {-2.0, -1.0, 0.0}
};

/* The XY coordinats for a 5-sided prism. */
Vector   prism5[5] = {
    { 0.5, -1.0, 0.0}, 
    {-1.0,  0.5, 0.0}, 
    { 1.0,  1.0, 0.0}, 
    {-1.0,  2.0, 0.0}, 
    {-2.0,  0.5, 0.0}
};

extern char *optarg;

int
main(int argc, char **argv)
{
    FILE      * fp ;
    Object    * prism3_obj;
    Object    * prism5_obj;
    Surf_desc   surf;

    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "prism.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "prism.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "prism.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "prism.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "prism.pbm";
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
    surf.color.red = 1.0000;    /* light salmon */
    surf.color.grn = 0.6275;
    surf.color.blu = 0.4784;
    surf.opacity.red = 1.0;
    surf.opacity.grn = 1.0;
    surf.opacity.blu = 1.0;

    /* The 3-sided prism */
    prism3_obj = sipp_prism(3, &prism3[0], 4.0, &surf, basic_shader, WORLD);
    object_move(prism3_obj, -3.0, 0.0, 0.0);
    object_add_subobj(sipp_world, prism3_obj);

    /* The 5-sided prism */
    prism5_obj = sipp_prism(5, &prism5[0], 5.0, &surf, basic_shader, WORLD);
    object_move(prism5_obj, 3.0, 0.0, 0.0);
    object_add_subobj(sipp_world, prism5_obj);

    /* The block (a 4 sided prism)  */
    object_add_subobj(sipp_world, sipp_block(1.0, 2.0, 3.0, &surf,
                                             basic_shader, WORLD));

    camera_params(sipp_camera, 5.0, -10.0, 6.0,  0.0, 0.0, 0.0,  
                  0.0, 0.0, 1.0,  0.4);

    printf("Rendering, wait...");
    fflush(stdout);

    fp = fopen(imfile_name, "w");
    render_image_file(size, size, fp, mode, 2);
    printf("Done.\n");

    exit(0);
}
