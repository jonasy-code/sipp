#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <sipp.h>
#include <shaders.h>
#include <primitives.h>


#define BEZ_RES        9
#define CYL_RES        40
#define LID_ROT        1.5
#define BLOCK_SIZE     1.2
#define NCYL           16
#define CYL_LEN        5.0
#define SMALL_CYL_RAD  ((BLOCK_SIZE * M_PI) / (NCYL * 2))
#define BIG_CYL_RAD    (0.5 * BLOCK_SIZE - 1.1 * SMALL_CYL_RAD)


Marble_desc teapot_surf = {
    0.4, 
    0.5,
    0.05,
    8.0, 
    {0.90, 0.80, 0.65}, 
    {0.30, 0.08, 0.08}, 
    {1.0, 1.0, 1.0}
};

Granite_desc column_surf = {
    0.4,
    0.1,
    0.4,
    20.0,
    {0.647, 0.565, 0.5},
    {0.15, 0.12, 0.10}, 
    {1.0, 1.0, 1.0}
};

extern char *optarg;

int
main(int argc, char **argv)
{
    Object  *column;
    Object  *teapot;
    Object  *tmp;
    FILE    *image;
    int      i;

    char    *imfile_name;
    int      mode;
    int      c;
    bool shade_once = FALSE;
    int      size;

    imfile_name = "isy90.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "apgfhls:")) != EOF) {
        switch (c) {
          case 'a': shade_once = TRUE; break;
          case 'p':
            mode = PHONG;
            imfile_name = "isy90.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "isy90.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "isy90.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "isy90.pbm";
            break;

          case 's':
            size = atoi(optarg);
            break;
        }
    }

    sipp_init();
    sipp_shading_per_pixel(shade_once);
    sipp_shadows(TRUE, ((size<512)?2*size:size));

    teapot = sipp_teapot(BEZ_RES, &teapot_surf, marble_shader, WORLD);
    object_add_subobj(sipp_world, teapot);

    column = object_create();
    tmp = sipp_block(BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE / 4.0, 
                     &column_surf, granite_shader, WORLD);
    object_move(tmp, 0.0, 0.0, -BLOCK_SIZE / 8.0);
    object_add_subobj(column, tmp);

    for (i = 0; i < NCYL; i++) {
        if (i == 0) {
            tmp = sipp_cylinder(SMALL_CYL_RAD, CYL_LEN, CYL_RES,
                                &column_surf, granite_shader, WORLD); 
        } else {
            tmp = object_instance(tmp);
        }
        object_move(tmp, BIG_CYL_RAD * cos(i * 2.0 * M_PI / NCYL), 
                    BIG_CYL_RAD * sin(i * 2.0 * M_PI / NCYL), 
                    -0.5 * (CYL_LEN + BLOCK_SIZE / 4.0));
        object_add_subobj(column, tmp);
    }

    object_add_subobj(sipp_world, column);
        
    lightsource_create(-3.0, -3.0, 6.0, 0.35, 0.35, 0.35, LIGHT_DIRECTION);
    spotlight_create(-3.0, -3.0, 6.0,  
                     0.0, 0.0, 0.0, 
                     25.0, 
                     0.45, 0.45, 0.45, 
                     SPOT_SHARP, TRUE);

    camera_position(sipp_camera, 2.0, -4.0, 1.5);
    camera_look_at(sipp_camera, 0.0, 0.0, 0.1);
    camera_up(sipp_camera, 0.0, 0.0, 1.0);
    camera_focal(sipp_camera, 0.2);

    printf("Rendering, wait...");
    fflush(stdout);

    image = fopen(imfile_name, "w");
    render_image_file(size, size, image, mode, 3);
    printf("Done.\n");

    exit(0);
}
