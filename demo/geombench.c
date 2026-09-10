/*
 * geombench.c - A geometry-heavy benchmark scene for SIPP.
 *
 * The regular demos are dominated by per-pixel shader cost (noise
 * textures, shadow filtering).  This scene is the opposite: many
 * finely tessellated tori shaded with the cheap basic_shader, so the
 * time goes into vertex transformation, clipping, edge creation and
 * the scanline sweep.  Use it to measure geometry-side optimizations.
 *
 * Options (in addition to the usual -p -g -f -l -s):
 *    -r n   tessellation factor (default 4).  Each torus gets
 *           (24*n) x (12*n) polygons, so polygon count scales as n^2.
 *    -c n   grid is n x n tori (default 4).
 *    -t     print the object build time and the render time separately.
 */

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <primitives.h>
#include <sipp.h>

extern char *optarg;

static double now(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv) {
  FILE *fp;
  Surf_desc surf;
  Surf_desc *sp;
  Object *torus;
  char imfile_name[64];
  const char *imbase;
  Render_mode mode;
  int c;
  bool shade_once = FALSE;
  int nthreads = 1;
  Image_format format = IMAGE_PPM;
  bool alpha = FALSE;
  int size;
  int res;
  int grid;
  int i, j;
  int report;
  long npolys;
  double t0, t1, t2;

  imbase = "geombench";
  mode = PHONG;
  size = 256;
  res = 4;
  grid = 4;
  report = 0;

  while ((c = getopt(argc, argv, "aj:pgfls:r:c:tPA")) != EOF) {
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
    case 'r':
      res = atoi(optarg);
      break;
    case 'c':
      grid = atoi(optarg);
      break;
    case 't':
      report = 1;
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

  surf.ambient = 0.4;
  surf.specular = 0.5;
  surf.c3 = 0.3;
  surf.opacity.red = surf.opacity.grn = surf.opacity.blu = 1.0;

  t0 = now();
  npolys = 0;
  for (i = 0; i < grid; i++) {
    for (j = 0; j < grid; j++) {
      surf.color.red = 0.3 + 0.6 * i / (double)grid;
      surf.color.grn = 0.3 + 0.6 * j / (double)grid;
      surf.color.blu = 0.6;
      /* sipp_torus() keeps a pointer to the descriptor rather */
      /* than a copy, so each torus needs its own.              */
      sp = (Surf_desc *)malloc(sizeof(Surf_desc));
      *sp = surf;
      torus = sipp_torus(1.0, 0.4, 24 * res, 12 * res, sp, basic_shader, WORLD);
      npolys += (long)(24 * res) * (12 * res);
      object_rot_x(torus, M_PI / 2.0 * ((i + j) % 2));
      object_rot_y(torus, 0.3 * i);
      object_move(torus, 3.0 * (i - (grid - 1) / 2.0),
                  3.0 * (j - (grid - 1) / 2.0), 0.0);
      object_add_subobj(sipp_world, torus);
    }
  }
  t1 = now();

  camera_params(sipp_camera, 0.0, -1.5 * grid, 2.5 * grid + 3.0, /* position */
                0.0, 0.0, 0.0,                                   /* look at  */
                0.0, 1.0, 0.0,                                   /* up       */
                0.5);

  fp = fopen(imfile_name, "w");
  render_image_file(size, size, fp, format, mode, 2);
  fclose(fp);
  t2 = now();

  if (report) {
    printf("geombench: %d x %d tori, %ld polygons\n", grid, grid, npolys);
    printf("  object build: %8.3f s\n", t1 - t0);
    printf("  render:       %8.3f s\n", t2 - t1);
  }

  exit(0);
}
