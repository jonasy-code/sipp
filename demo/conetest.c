#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <primitives.h>
#include <sipp.h>

#define RESOLUTION 20

extern char *optarg;

int main(int argc, char **argv) {
  FILE *fp;
  Object *cone;
  Object *trunc_cone;
  Surf_desc surf;

  char imfile_name[64];
  const char *imbase;
  int mode;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  Image_format format = IMAGE_PPM;
  bool alpha = FALSE;
  int size;

  imbase = "cone";
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

  lightsource_create(1.0, 1.0, 1.0, 0.9, 0.9, 0.9, LIGHT_DIRECTION);
  lightsource_create(-1.0, -1.0, 0.5, 0.4, 0.4, 0.4, LIGHT_DIRECTION);

  surf.ambient = 0.5;
  surf.specular = 0.6;
  surf.c3 = 0.2;
  surf.color.red = 1.0000; /* light salmon */
  surf.color.grn = 0.6275;
  surf.color.blu = 0.4784;
  surf.opacity.red = 1.0;
  surf.opacity.grn = 1.0;
  surf.opacity.blu = 1.0;

  /* The ordinary cone */
  cone = sipp_cone(1.0, 0.0, 4.0, RESOLUTION, &surf, basic_shader, WORLD);
  object_move(cone, -3.0, 0.0, 0.0);
  object_add_subobj(sipp_world, cone);

  /* The truncated cone */
  trunc_cone = sipp_cone(1.0, 0.4, 5.0, RESOLUTION, &surf, basic_shader, WORLD);
  object_move(trunc_cone, 3.0, 0.0, 0.0);
  object_add_subobj(sipp_world, trunc_cone);

  /* The cylinder (a trucated cone with equal top and bottom radii */
  object_add_subobj(sipp_world, sipp_cylinder(1.0, 3.0, RESOLUTION, &surf,
                                              basic_shader, WORLD));

  camera_params(sipp_camera, 5.0, -10.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0,
                0.4);

  printf("Rendering, wait...");
  fflush(stdout);

  fp = fopen(imfile_name, "w");
  render_image_file(size, size, fp, format, mode, 2);
  printf("Done.\n");

  exit(0);
}
