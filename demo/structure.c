#include <stdio.h>
#include <math.h>

#include <sipp.h>
#include <primitives.h>
#include <shaders.h>


#define SPHERERES 40
#define CYLRES    40

#define SIGNBIT(bit, i)   (((i >> bit) & 1) ? -1.0 : 1.0)

Surf_desc surf = {
    0.4,
    0.7, 
    0.1, 
    {0.8, 0.6, 0.3}, 
    {1.0, 1.0, 1.0}
    };
    
Bumpy_desc bumpy_surf = {
    basic_shader, 
    &surf, 
    14.0, 
    FALSE, 
    TRUE
};

extern char *optarg;

main(argc, argv)
    int    argc;
    char **argv;
{
    Object  *sphere;
    Object  *cyl;
    Object  *structure;
    FILE    *fp ;
    Surf_desc cyl_surf;
    int      i;

    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "structure.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "structure.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "structure.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "structure.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "structure.pbm";
            break;

          case 's':
            size = atoi(optarg);
            break;
        }
    }

    sipp_init();

    lightsource_create(1.0, 1.0, 1.0, 0.9, 0.9, 0.9, LIGHT_DIRECTION);
    lightsource_create(-1.0, -1.0, 0.5, 0.4, 0.4, 0.4, LIGHT_DIRECTION);

    cyl_surf.ambient = 0.5;
    cyl_surf.specular = 0.4;
    cyl_surf.c3 = 0.3;
    cyl_surf.color.red = 0.5;
    cyl_surf.color.grn = 0.6;
    cyl_surf.color.blu = 0.8;
    cyl_surf.opacity.red = 1.0;
    cyl_surf.opacity.grn = 1.0;
    cyl_surf.opacity.blu = 1.0;
    
    structure = object_create();

    sphere = sipp_sphere(1.0, SPHERERES, &bumpy_surf, bumpy_shader, WORLD);
    for (i = 0; i < 8; i++) {
        if (i) {
            sphere = object_instance(sphere);
        }
        object_move(sphere, 2.0 * SIGNBIT(2, i), 2.0 * SIGNBIT(1, i), 
                    2.0 * SIGNBIT(0, i));
        object_add_subobj(structure, sphere);
    }

    cyl = sipp_cylinder(0.25, 4.0, CYLRES, &cyl_surf, basic_shader, WORLD);
    for (i = 0; i < 4; i++) {
        if (i) {
            cyl = object_instance(cyl);
        }
        object_move(cyl, 2.0 * SIGNBIT(1, i), 2.0 * SIGNBIT(0, i), 0.0);
        object_add_subobj(structure, cyl);
    }
    for (i = 0; i < 4; i++) {
        cyl = object_instance(cyl);
        object_rot_x(cyl, M_PI / 2.0);
        object_move(cyl, 2.0 * SIGNBIT(1, i), 0.0, 2.0 * SIGNBIT(0, i));
        object_add_subobj(structure, cyl);
    }
    for (i = 0; i < 4; i++) {
        cyl = object_instance(cyl);
        object_rot_y(cyl, M_PI / 2.0);
        object_move(cyl, 0.0, 2.0 * SIGNBIT(1, i), 2.0 * SIGNBIT(0, i));
        object_add_subobj(structure, cyl);
    }
    
    object_add_subobj(sipp_world, structure);

    camera_position(sipp_camera, 10.0, -5.0, 15.0);
    camera_look_at(sipp_camera, 0.0, 0.0, 0.0);
    camera_up(sipp_camera, 0.0, 0.0, 1.0);
    camera_focal(sipp_camera, 0.25);

    printf("Rendering, wait...");
    fflush(stdout);

    fp = fopen(imfile_name, "w");
    render_image_file(size, size, fp, mode, 2);
    printf("Done.\n");

    exit(0);
}
