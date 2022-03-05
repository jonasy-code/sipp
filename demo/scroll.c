#include <stdio.h>
#include <math.h>

#include <sipp.h>


Surf_desc scroll_surf = {
    0.3, 
    0.0,
    0.1, 
    {0.85, 0.85, 0.35}, 
    {1.0, 1.0, 1.0},
};

/*
 * Shader to map a pbm bitmap (ugly hardcoded size...)
 * onto the scroll. We map it on the middle flat part.
 */
unsigned char scrolltexture[360][113];
int scroll_ready = 0;
void
scroll_shader(pos, normal, texture, view_vec, lights, foo, color, opacity)
    Vector      *pos;
    Vector      *normal;
    Vector      *texture;
    Vector      *view_vec;
    Lightsource *lights;
    Surf_desc   *foo;
    Color       *color;
    Color       *opacity;
{
    Surf_desc   sd;
    FILE       *texture_file;
    int         x, y;

    if (!scroll_ready) {
        texture_file = fopen("sipp.bm", "r");
        fscanf(texture_file, "P4\n%d %d\n", &x, &y);
        if (x != 900 || y != 360) {
            puts("FOO!");
            exit(1);
        }
        fread(scrolltexture, 1, 113*360, texture_file);
        fclose(texture_file);
        scroll_ready = 1;
    }

    sd = scroll_surf;

    if (texture->x > 0.2 && texture->x < 0.8) {
        x = (texture->x - 0.2) / 0.6 * 900;
        y = texture->y * 360;
        if (scrolltexture[y][x / 8] & (1 << (7 - (x % 8)))) {
            sd.color.red = sd.color.grn = sd.color.blu = 0.0;
        }
    }
    
    basic_shader(pos, normal, texture, view_vec, lights, &sd, 
                 color, opacity); 
}


/*
 * FFD which twists the ends of the scroll "paper"
 */
void
scroll_twist(dummy, world, txt, new_world, new_txt)
    char *dummy;
    Vector *world, *txt, *new_world, *new_txt;
{
    double rad;
    double ang;

    if (txt->x < 0.2) {
        ang = (world->x + 2.5) * 3.0 * M_PI / 2.0;
        rad = (world->x + 2.5) / 4.0;
        new_world->x = cos(ang) * rad - 1.5;
        new_world->z = -sin(ang) * rad - 0.25;
        new_world->y = world->y + 0.2 - (world->x + 2.5) * 0.2;
    } else if (txt->x > 0.8) {
        ang = (2.5 - world->x) * 3.0 * M_PI / 2.0;
        rad = (2.5 - world->x) / 4.0;
        new_world->x = -cos(ang) * rad + 1.5;
        new_world->z = sin(ang) * rad + 0.25;
        new_world->y = world->y - 0.2 + (2.5 - world->x) * 0.2;
    } else {
        *new_world = *world;
    }
    *new_txt = *txt;
}


extern char *optarg;

main(argc, argv)
    int argc;
    char **argv;
{
    Object  *scroll;
    Surface *scrollsurf;
    int    i;

    FILE    *image;
    char    *imfile_name;
    int      mode;
    int      c;
    int      size;

    imfile_name = "scroll.ppm";
    mode = PHONG;
    size = 256;

    while ((c = getopt(argc, argv, "pgfls:")) != EOF) {
        switch (c) {
          case 'p':
            mode = PHONG;
            imfile_name = "scroll.ppm";
            break;

          case 'g':
            mode = GOURAUD;
            imfile_name = "scroll.ppm";
            break;

          case 'f':
            mode = FLAT;
            imfile_name = "scroll.ppm";
            break;

          case 'l':
            mode = LINE;
            imfile_name = "scroll.pbm";
            break;

          case 's':
            size = atoi(optarg);
            break;
        }
    }

    sipp_init();
    sipp_show_backfaces(TRUE);
    sipp_background(0.078, 0.361, 0.753); /* UNC sky blue */
    sipp_shadows(TRUE, (size<512)?2*size:size);
    lightsource_create(3.0, 2.0, 6.0, 0.35, 0.35, 0.35, LIGHT_DIRECTION);
    spotlight_create(3.0, 2.0, 6.0,  
                     0.0, 0.0, 0.0, 
                     25.0, 
                     0.45, 0.45, 0.45, 
                     SPOT_SOFT, TRUE);

    /*
     * Create the scroll object
     */
    scroll = object_create();

    /*
     * Create a surface which is only a long flat 
     * rectangle made of smaller rectangles. 
     * It will be twisted to its final form with an ffd.
     */
    for (i = 0; i < 100; i++) {
        vertex_tx_push(i / 20.0 - 2.5,  -0.5, 0.0,       i / 100.0, 1.0, 0.0);
        vertex_tx_push(i / 20.0 - 2.45, -0.5, 0.0, (i + 1) / 100.0, 1.0, 0.0);
        vertex_tx_push(i / 20.0 - 2.45,  0.5, 0.0, (i + 1) / 100.0, 0.0, 0.0);
        vertex_tx_push(i / 20.0 - 2.5,   0.5, 0.0,       i / 100.0, 0.0, 0.0);
        polygon_push();
    }
    scrollsurf = surface_create(&scroll_surf, scroll_shader);
    surface_set_ffd(scrollsurf, scroll_twist, NULL);
    object_add_surface(scroll, scrollsurf);

    object_rot_x(scroll, -0.1);
    object_rot_z(scroll, 0.2);
    object_add_subobj(sipp_world, scroll);

    camera_params(sipp_camera, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0,  
                  0.0, 1.0, 0.0,  0.125);

    printf("Rendering, wait...");
    fflush(stdout);

    image = fopen(imfile_name, "w");
    render_image_file(size, size, image, mode, 3);
    printf("Done.\n");

    exit(0);
}
