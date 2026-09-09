#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <sipp.h>
#include <geometric.h>
#include <shaders.h>
#include <primitives.h>

extern double atof();

extern char *optarg;
extern int optind,  opterr;


#define RESOLUTION    5
#define FLOORSIZE     15.0


Marble_desc teapot_surf = {
    0.4, 
    0.5, 
    0.05, 
    8.0, 
    {0.90, 0.80, 0.65}, 
    {0.30, 0.08, 0.08}, 
    {1.0, 1.0, 1.0}
};


typedef struct {
    double   sqsize;
    Surf_desc   col1;
    Surf_desc   col2;
} Floor_desc;


Floor_desc floor_surf = {
    1.0,
    { 0.4, 0.0, 0.1, {0.9900, 0.9000, 0.7900}, {1.0, 1.0, 1.0} },
    { 0.4, 0.0, 0.1, {0.8300, 0.2400, 0.1000}, {1.0, 1.0, 1.0} }
};


/*
 * A shader to produce a checkered floor.
 */
static void
floor_shader(Vector *pos, Vector *normal, Vector *texture, Vector *view_vec, Lightsource *lights, Floor_desc *fd, Color *color, Color *transp)
{
    Surf_desc  * col;
    int          intu;
    int          intv;

    intu = floor(texture->x / fd->sqsize);
    if (intu < 0)
        intu = -intu;

    intv = floor(texture->y / fd->sqsize);
    if (intv < 0)
        intv = -intv;

    if ((intu ^ intv) & 1)
        col = &fd->col1;
    else
        col = &fd->col2;

    basic_shader(pos, normal, texture, view_vec, lights, col, color, transp);
}



int
main(int argc, char **argv)
{
    Object  *teapot;		/* The teapot and its bottom */
    Object  *bottom;
    Transf_mat  * teapot_transf;

    Object  *floor;		/* The floor. */

    int      image_size;
    FILE    *image;
    int      frame;
    int      mode;

    double   time_start;	/* Animation time values. */
    double   time_stop;
    double   time_step;
    double   time;
    double   t_jump;
    double   height;
    double   t_scale;
    double   xyscaling;
    double   zscaling;
    double   angle;
    char     filename[256];
    char     *file_ext;
    char     c;

    mode = LINE;
    time_start = 0.0;
    time_stop  = 1.0;
    time_step  = 0.04;
    image_size = 256;
    file_ext = "pbm";

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            file_ext = "ppm";
            break;

          case 'g':
            mode = GOURAUD;
            file_ext = "ppm";
            break;

          case 'f':
            mode = FLAT;
            file_ext = "ppm";
            break;

          case 'l':
            mode = LINE;
            file_ext = "pbm";
            break;

          case 's':
            image_size = atoi(optarg);
            break;
        }
    }


    sipp_init();


    /* Create the floor. */
    floor = sipp_block(FLOORSIZE, FLOORSIZE, 1.0, &floor_surf, floor_shader,
                       WORLD);
    object_move(floor, 0.0, 0.0, -0.5);
    object_add_subobj(sipp_world, floor);


    /* Create the teapot and its bottom. */
    teapot = sipp_teapot(RESOLUTION, &teapot_surf, marble_shader, WORLD);
    bottom = sipp_cylinder(0.375, 0.01, RESOLUTION * 4, &teapot_surf,
                           marble_shader, WORLD);
    object_add_subobj(teapot, bottom);
    object_add_subobj(sipp_world, teapot);


    /* Lit the stage! */
    lightsource_create(0.2, -2.0, 1.0, 1.0, 1.0, 1.0, LIGHT_DIRECTION);
    lightsource_create(1.0, 0.0, 0.5, 0.4, 0.4, 0.4, LIGHT_DIRECTION);


    /* Viewing parameters. */
    camera_position(sipp_camera, 16.0, -24.0, 4.0);
    camera_look_at(sipp_camera, 0.0, 0.0, 1.4);
    camera_up(sipp_camera, 0.0, 0.0, 1.0);
    camera_focal(sipp_camera, 0.0625);


    /*
     * The following code is quite ugly and full of magic numbers.
     * It is basically two parabolas that describe the teapots jump 
     * and its "squashing" when it lands.
     */
    frame = time_start / time_step;
    time = frame * time_step;
    while (time < time_stop) {

        if (time < 0.65) {
	    /* During the jump */
            t_jump = time * 1.94464;
            height = 6.2 * t_jump - 9.81 * t_jump * t_jump / 2.0;
            angle = 2.0 * M_PI * time / 0.65;
            xyscaling = 1.0;
            zscaling = 1.0;

        } else {
	    /* During the squashing phase. */
            height = 0.0;
            angle = 0.0;
            t_scale = (time - 0.65) * 0.64533;
            xyscaling = 1.0 + (1.0 - zscaling) * M_SQRT1_2;
            zscaling = 1.0 - 6.2 * t_scale + 54.9 * t_scale * t_scale / 2.0;
        }

	/* 
	 * Save the original transformation state of the teapot.
	 * It is easier to recreate the proper position from scratch
	 * for each new frame than to calculate the difference between
	 * one image and the next.
	 */
	teapot_transf = object_get_transf(teapot, NULL);

	/* Place the teapot in its proper position. */
        object_scale(teapot, xyscaling, xyscaling, zscaling);
        object_move(teapot, 0.0, 0.0, -0.4);
        object_rot_y(teapot, -angle);
        object_move(teapot, 0.0, 0.0, 0.4);
        object_move(teapot, 0.0, 0.0, height);

        sprintf(filename, "anim%02d.%s", frame, file_ext);
        image = fopen(filename, "w");
        printf("\rRendering frame %2d...", frame);
        fflush(stdout);

	/* Render the image. */
        render_image_file(image_size, image_size, image, mode, 3);
        fclose(image);

	/* Reset the teapot to its original position and shape. */
	object_set_transf(teapot, teapot_transf);

        frame++;
        time = frame * time_step;
    }

    printf("done.\n");
    exit(0);
}
