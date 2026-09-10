#include <stdio.h>
#include <unistd.h>

#include <sipp.h>

#include <filter.h>
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

static void hole_shader(const Shade_point *sp, Lightsource *lights, void *sd_,
                        Color *color, Color *transp) {
  Surf_desc surf = *(Surf_desc *)sd_; /* Local copy: a shader must
                                         not write into the shared
                                         surface description */
  Surf_desc *sd = &surf;
  Vector tmp, grad;
  double width, n, solid;

  noise_init();

  VecCopy(tmp, sp->texture);
  VecScalMul(tmp, 35.0, tmp);
  width = shade_texture_width(sp) * 35.0;

  /*
   * The surface has a hole where the noise is below -0.1.  Noise finer
   * than the sample is left out, and the edge of a hole is anti-aliased
   * by taking the fraction of the sample that is solid: the step at
   * -0.1, filtered over how much the noise changes across the sample.
   */
  n = noise_filtered(&tmp, width);
  grad = Dnoise(&tmp);
  solid = filter_step(-0.1, n, VecLen(grad) * width);
  sd->opacity.red = solid;
  sd->opacity.grn = solid;
  sd->opacity.blu = solid;

  basic_shader(sp, lights, sd, color, transp);
}

/*
 * A shader to produce a checkered floor.  The board is averaged over
 * the area the sample covers (filter_checker()), so that it turns into
 * an even grey at a distance instead of a moire pattern; the sample's
 * extent in u and v is the size of the box around its footprint.
 */
static void floor_shader(const Shade_point *sp, Lightsource *lights, void *fd_,
                         Color *color, Color *transp) {
  Floor_desc *fd = (Floor_desc *)fd_;
  Surf_desc surf;
  double mix;

  /* The fraction of the sample that is col1: squares of odd parity. */
  mix = filter_checker(sp->texture.x / fd->sqsize, sp->texture.y / fd->sqsize,
                       (fabs(sp->dtdx.x) + fabs(sp->dtdy.x)) / fd->sqsize,
                       (fabs(sp->dtdx.y) + fabs(sp->dtdy.y)) / fd->sqsize);

  if (mix >= 1.0) {
    surf = fd->col1;
  } else if (mix <= 0.0) {
    surf = fd->col2;
  } else {
    surf = fd->col1;
    surf.color.red =
        fd->col2.color.red + mix * (fd->col1.color.red - fd->col2.color.red);
    surf.color.grn =
        fd->col2.color.grn + mix * (fd->col1.color.grn - fd->col2.color.grn);
    surf.color.blu =
        fd->col2.color.blu + mix * (fd->col1.color.blu - fd->col2.color.blu);
  }

  basic_shader(sp, lights, &surf, color, transp);
}

extern char *optarg;

int main(int argc, char **argv) {
  Object *teapot;
  Object *bottom;
  FILE *image;

  Object *floor;

  char imfile_name[64];
  const char *imbase;
  Render_mode mode;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  Image_format format = IMAGE_PPM;
  bool alpha = FALSE;
  int size;

  imbase = "teapot";
  mode = PHONG;
  size = 256;

  while ((c = getopt(argc, argv, "aj:pgfls:PA")) != EOF) {
    switch (c) {
    case 'a':
      shade_once = TRUE;
      break;
    case 'j':
      nthreads = atoi(optarg);
      break;
    case 'p':
      mode = PHONG;
      break;

    case 'g':
      mode = GOURAUD;
      break;

    case 'f':
      mode = FLAT;
      break;

    case 'l':
      mode = LINE;
      break;

    case 's':
      size = atoi(optarg);
      break;

    case 'P':
      format = IMAGE_PNG;
      break;

    case 'A':
      alpha = TRUE;
      break;
    }
  }

  if (alpha) {
    format = (Image_format)(format | IMAGE_ALPHA);
  }
  snprintf(imfile_name, sizeof(imfile_name), "%s.%s", imbase,
           sipp_image_extension(format, mode));

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
  render_image_file(size, size, image, format, mode, 3);
  printf("Done.\n");

  exit(0);
}
