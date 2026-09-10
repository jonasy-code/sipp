/**
 ** animation.c - Render the somersaulting teapot (scene.c) as a sequence
 **               of image files, anim00.ppm, anim01.ppm, ...
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sipp.h>

#include "scene.h"

extern char *optarg;
extern int optind, opterr;

int main(int argc, char **argv) {
  int image_size;
  FILE *image;
  int frame;
  Render_mode mode;
  char filename[256];
  const char *file_ext;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  Image_format format = IMAGE_PPM;
  bool alpha = FALSE;

  mode = LINE;
  image_size = 256;

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
      image_size = atoi(optarg);
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
  file_ext = sipp_image_extension(format, mode);

  sipp_init();
  sipp_shading_per_pixel(shade_once);
  sipp_render_threads(nthreads);

  anim_scene_create((image_size < 512) ? 2 * image_size : image_size);

  for (frame = 0; frame < anim_frames(); frame++) {
    anim_scene_place(frame * anim_time_step);

    sprintf(filename, "anim%02d.%s", frame, file_ext);
    image = fopen(filename, "w");
    printf("\rRendering frame %2d...", frame);
    fflush(stdout);

    render_image_file(image_size, image_size, image, format, mode, 3);
    fclose(image);
  }

  printf("done.\n");
  exit(0);
}
