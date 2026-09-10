#include <stdio.h>
#include <unistd.h>

#include <sipp.h>

#include <geometric.h>
#include <noise.h>
#include <primitives.h>
#include <shaders.h>

#define RESOLUTION 9

Surf_desc teapot_surf = {
    0.4, 0.5, 0.1, {0.9, 0.6, 0.6}, {0.45, 0.45, 0.45},
};

typedef struct {
  double sqsize;
  Surf_desc col1;
  Surf_desc col2;
} Floor_desc;

Floor_desc floor_surf = {
    1.0,
    {0.4, 0.0, 0.1, {0.9900, 0.9000, 0.7900}, {1.0, 1.0, 1.0}},
    {0.4, 0.0, 0.1, {0.8300, 0.2400, 0.1000}, {1.0, 1.0, 1.0}}};

static void hole_shader(Vector *pos, Vector *normal, Vector *texture,
                        Vector *view_vec, Lightsource *lights, void *sd_,
                        Color *color, Color *transp) {
  Surf_desc surf = *(Surf_desc *)sd_; /* Local copy: a shader must
                                         not write into the shared
                                         surface description */
  Surf_desc *sd = &surf;
  Vector tmp;

  noise_init();

  VecCopy(tmp, *texture);
  VecScalMul(tmp, 35.0, tmp);

  if (noise(&tmp) < -0.1) {
    sd->opacity.red = 0.0;
    sd->opacity.grn = 0.0;
    sd->opacity.blu = 0.0;
  } else {
    sd->opacity.red = 1.0;
    sd->opacity.grn = 1.0;
    sd->opacity.blu = 1.0;
  }

  basic_shader(pos, normal, texture, view_vec, lights, sd, color, transp);
}

/*
 * A shader to produce a checkered floor.
 */
static void floor_shader(Vector *pos, Vector *normal, Vector *texture,
                         Vector *view_vec, Lightsource *lights, void *fd_,
                         Color *color, Color *transp) {
  Floor_desc *fd = (Floor_desc *)fd_;
  Surf_desc *col;
  int intu;
  int intv;

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

extern char *optarg;

int main(int argc, char **argv) {
  Object *teapot;
  Object *bottom;
  FILE *image;

  Object *floor;

  char *imfile_name;
  int mode;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  int size;

  imfile_name = "teapot.ppm";
  mode = PHONG;
  size = 256;

  while ((c = getopt(argc, argv, "aj:pgfls:")) != EOF) {
    switch (c) {
    case 'a':
      shade_once = TRUE;
      break;
    case 'j':
      nthreads = atoi(optarg);
      break;
    case 'p':
      mode = PHONG;
      imfile_name = "teapot.ppm";
      break;

    case 'g':
      mode = GOURAUD;
      imfile_name = "teapot.ppm";
      break;

    case 'f':
      mode = FLAT;
      imfile_name = "teapot.ppm";
      break;

    case 'l':
      mode = LINE;
      imfile_name = "teapot.pbm";
      break;

    case 's':
      size = atoi(optarg);
      break;
    }
  }

  sipp_init();
  sipp_shading_per_pixel(shade_once);
  sipp_render_threads(nthreads);
  sipp_show_backfaces(TRUE);
  sipp_background(0.078, 0.361, 0.753); /* UNC sky blue */

  lightsource_create(-3.0, -2.0, 6.0, 0.35, 0.35, 0.35, LIGHT_DIRECTION);
  spotlight_create(-3.0, -2.0, 6.0, 0.0, 0.0, 0.0, 25.0, 0.45, 0.45, 0.45,
                   SPOT_SOFT, TRUE);

  teapot = sipp_teapot(RESOLUTION, &teapot_surf, hole_shader, WORLD);
  bottom = sipp_cylinder(0.375, 0.01, RESOLUTION * 4, &teapot_surf, hole_shader,
                         WORLD);
  object_move(bottom, 0.0, 0.0, 0.005);
  object_add_subobj(teapot, bottom);

  object_add_subobj(sipp_world, teapot);

  /*
   * The floor will never cast a shadow on anything so
   * we render the shadowmaps before creating it.
   */
  shadowmaps_create((size < 512) ? 2 * size : size);

  floor = sipp_block(7.0, 7.0, 0.2, &floor_surf, floor_shader, WORLD);
  object_move(floor, 0.0, 0.0, -0.1);
  object_add_subobj(sipp_world, floor);

  camera_params(sipp_camera, 1.65, -7.7, 3.3, 0.0, 0.0, 0.4, 0.0, 0.0, 1.0,
                0.125);

  printf("Rendering, wait...");
  fflush(stdout);

  image = fopen(imfile_name, "w");
  render_image_file(size, size, image, mode, 3);
  printf("Done.\n");

  exit(0);
}
