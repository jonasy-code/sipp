#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <primitives.h>
#include <shaders.h>
#include <sipp.h>

Wood_desc wood_surf = {0.5,
                       0.0,
                       0.99,
                       10.0,
                       {0.770, 0.568, 0.405},
                       {0.468, 0.296, 0.156},
                       {1.0, 1.0, 1.0}};

extern char *optarg;

int main(int argc, char **argv) {
  FILE *fp;

  char imfile_name[64];
  const char *imbase;
  Render_mode mode;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  Image_format format = IMAGE_PPM;
  bool alpha = FALSE;
  int size;

  imbase = "wood";
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

  object_add_subobj(sipp_world,
                    sipp_block(4.0, 3.0, 3.0, &wood_surf, wood_shader, WORLD));

  camera_params(sipp_camera, 10.0, 10.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                0.125);

  printf("Rendering, wait...");
  fflush(stdout);

  fp = fopen(imfile_name, "w");
  render_image_file(size, size, fp, format, mode, 3);
  printf("Done.\n");

  exit(0);
}
