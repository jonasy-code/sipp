#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <primitives.h>
#include <shaders.h>
#include <sipp.h>

#define SUBDIVS 50

extern char *optarg;

int main(int argc, char **argv) {
  Surf_desc planet_surface;
  FILE *outfile;

  char imfile_name[64];
  const char *imbase;
  Render_mode mode;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  Image_format format = IMAGE_PPM;
  bool alpha = FALSE;
  int size;

  imbase = "planet";
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

  planet_surface.ambient = 0.4;
  planet_surface.specular = 0.0;
  planet_surface.c3 = 0.5;
  planet_surface.color.red = 1.0;
  planet_surface.color.grn = 0.0;
  planet_surface.color.blu = 0.0;
  planet_surface.opacity.red = 1.0;
  planet_surface.opacity.grn = 1.0;
  planet_surface.opacity.blu = 1.0;

  if (alpha) {
    format = (Image_format)(format | IMAGE_ALPHA);
  }
  snprintf(imfile_name, sizeof(imfile_name), "%s.%s", imbase,
           sipp_image_extension(format, mode));

  sipp_init();
  sipp_shading_per_pixel(shade_once);
  sipp_render_threads(nthreads);

  lightsource_create(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, LIGHT_DIRECTION);

  object_add_subobj(sipp_world, sipp_sphere(1.0, SUBDIVS, &planet_surface,
                                            planet_shader, WORLD));
  object_rot_z(sipp_world, -1.2);
  object_rot_x(sipp_world, 0.2);

  camera_params(sipp_camera, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.75);

  printf("Rendering, wait...");
  fflush(stdout);

  outfile = fopen(imfile_name, "w");
  render_image_file(size, size, outfile, format, mode, 3);
  printf("Done.\n");

  exit(0);
}
