#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <sipp.h>
#include <sipp_texture.h>

Surf_desc scroll_surf = {
    0.3, 0.0, 0.1, {0.85, 0.85, 0.35}, {1.0, 1.0, 1.0},
};

/*
 * Shader to map a pbm bitmap (ugly hardcoded size...)
 * onto the scroll. We map it on the middle flat part.
 */
unsigned char scrolltexture[360][113];

/*
 * The text as a one channel Sipp_texture (1.0 where the bitmap is
 * white), looked up filtered over the area the sample covers, so that
 * its edges are anti-aliased however small it gets.  It is spread over
 * the middle 60% of the paper.
 */
static Sipp_texture *scroll_tex;
/*
 * Read the bitmap used as texture.  Done once, before rendering: the
 * shader may run in several threads at once and must not load it.
 */
static void scroll_texture_load(void) {
  FILE *texture_file;
  int x, y, c;

  texture_file = fopen("sipp.bm", "r");
  if (texture_file == NULL) {
    fprintf(stderr, "scroll: cannot open texture file sipp.bm\n");
    exit(1);
  }
  x = y = 0;
  if (fscanf(texture_file, "P4") == EOF) {
    fprintf(stderr, "scroll: sipp.bm is not a P4 pbm file\n");
    exit(1);
  }
  /* Skip whitespace and any '#' comment lines before the size. */
  for (;;) {
    c = fgetc(texture_file);
    if (c == '#') {
      while (c != '\n' && c != EOF) {
        c = fgetc(texture_file);
      }
    } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      ungetc(c, texture_file);
      break;
    }
  }
  if (fscanf(texture_file, "%d %d", &x, &y) != 2) {
    fprintf(stderr, "scroll: cannot read bitmap size from sipp.bm\n");
    exit(1);
  }
  fgetc(texture_file); /* single whitespace before data */
  if (x != 900 || y != 360) {
    fprintf(stderr, "scroll: expected a 900x360 bitmap, got %dx%d\n", x, y);
    exit(1);
  }
  if (fread(scrolltexture, 1, 113 * 360, texture_file) != 113 * 360) {
    fprintf(stderr, "scroll: sipp.bm is truncated\n");
    exit(1);
  }
  fclose(texture_file);

  {
    Sipp_bitmap bm;

    bm.width = 900;
    bm.height = 360;
    bm.width_bytes = 113;
    bm.buffer = &scrolltexture[0][0];
    scroll_tex = sipp_texture_from_bitmap(&bm);
    scroll_tex->wrap = FALSE;
  }
}

static void scroll_shader(const Shade_point *sp, Lightsource *lights, void *foo,
                          Color *color, Color *opacity) {
  Surf_desc sd;
  double white;

  sd = scroll_surf;

  if (sp->texture.x > 0.2 && sp->texture.x < 0.8) {
    sipp_texture_sample(scroll_tex, (sp->texture.x - 0.2) / 0.6, sp->texture.y,
                        sp->dtdx.x / 0.6, sp->dtdx.y, sp->dtdy.x / 0.6,
                        sp->dtdy.y, &white);
    sd.color.red *= white;
    sd.color.grn *= white;
    sd.color.blu *= white;
  }

  basic_shader(sp, lights, &sd, color, opacity);
}

/*
 * FFD which twists the ends of the scroll "paper"
 */
static void scroll_twist(void *dummy, Vector *world, Vector *txt,
                         Vector *new_world, Vector *new_txt) {
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

int main(int argc, char **argv) {
  bool shade_once = FALSE;
  int nthreads = 1;
  Image_format format = IMAGE_PPM;
  bool alpha = FALSE;
  Object *scroll;
  Surface *scrollsurf;
  int i;

  FILE *image;
  char imfile_name[64];
  const char *imbase;
  Render_mode mode;
  int c;
  int size;

  imbase = "scroll";
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
  sipp_shadows(TRUE, (size < 512) ? 2 * size : size);
  lightsource_create(3.0, 2.0, 6.0, 0.35, 0.35, 0.35, LIGHT_DIRECTION);
  spotlight_create(3.0, 2.0, 6.0, 0.0, 0.0, 0.0, 25.0, 0.45, 0.45, 0.45,
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
    vertex_tx_push(i / 20.0 - 2.5, -0.5, 0.0, i / 100.0, 1.0, 0.0);
    vertex_tx_push(i / 20.0 - 2.45, -0.5, 0.0, (i + 1) / 100.0, 1.0, 0.0);
    vertex_tx_push(i / 20.0 - 2.45, 0.5, 0.0, (i + 1) / 100.0, 0.0, 0.0);
    vertex_tx_push(i / 20.0 - 2.5, 0.5, 0.0, i / 100.0, 0.0, 0.0);
    polygon_push();
  }
  scrollsurf = surface_create(&scroll_surf, scroll_shader);
  surface_set_ffd(scrollsurf, scroll_twist, NULL);
  object_add_surface(scroll, scrollsurf);

  object_rot_x(scroll, -0.1);
  object_rot_z(scroll, 0.2);
  object_add_subobj(sipp_world, scroll);

  camera_params(sipp_camera, 0.0, 0.0, 15.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                0.125);

  printf("Rendering, wait...");
  fflush(stdout);

  image = fopen(imfile_name, "w");
  scroll_texture_load();
  render_image_file(size, size, image, format, mode, 3);
  printf("Done.\n");

  exit(0);
}
