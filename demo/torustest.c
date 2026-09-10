#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <primitives.h>
#include <sipp.h>

#define SMALLRES 15
#define BIGRES 40

extern char *optarg;

int main(int argc, char **argv) {
  FILE *fp;
  Surf_desc surf;

  char *imfile_name;
  int mode;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  int size;

  imfile_name = "torus.ppm";
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
      imfile_name = "torus.ppm";
      break;

    case 'g':
      mode = GOURAUD;
      imfile_name = "torus.ppm";
      break;

    case 'f':
      mode = FLAT;
      imfile_name = "torus.ppm";
      break;

    case 'l':
      mode = LINE;
      imfile_name = "torus.pbm";
      break;

    case 's':
      size = atoi(optarg);
      break;
    }
  }

  sipp_init();
  sipp_shading_per_pixel(shade_once);
  sipp_render_threads(nthreads);

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

  object_add_subobj(sipp_world, sipp_torus(1.0, 0.4, BIGRES, SMALLRES, &surf,
                                           basic_shader, WORLD));

  camera_params(sipp_camera, 4.0, 0.0, 4.5, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.4);

  printf("Rendering, wait...");
  fflush(stdout);

  fp = fopen(imfile_name, "w");
  render_image_file(size, size, fp, mode, 2);
  printf("Done.\n");

  exit(0);
}
