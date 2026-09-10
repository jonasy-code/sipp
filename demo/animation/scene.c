/**
 ** scene.c - The somersaulting marble teapot of the animation demo.
 **           See scene.h.
 **/

#include <math.h>

#include <sipp.h>

#include <filter.h>
#include <geometric.h>
#include <primitives.h>
#include <shaders.h>

#include "scene.h"

enum { RESOLUTION = 5 };
static const double FLOORSIZE = 15.0;

const double anim_time_stop = 1.0;
const double anim_time_step = 0.04;

static Marble_desc teapot_surf = {0.4,
                                  0.5,
                                  0.05,
                                  8.0,
                                  {0.90, 0.80, 0.65},
                                  {0.30, 0.08, 0.08},
                                  {1.0, 1.0, 1.0}};

typedef struct {
  double sqsize;
  Surf_desc col1;
  Surf_desc col2;
} Floor_desc;

static Floor_desc floor_surf = {
    1.0,
    {0.3, 0.0, 0.1, {0.9900, 0.9000, 0.7900}, {1.0, 1.0, 1.0}},
    {0.3, 0.0, 0.1, {0.8300, 0.2400, 0.1000}, {1.0, 1.0, 1.0}}};

static Object *teapot; /* The teapot, with its bottom as a subobject */

/* The default camera, and the point it looks at. */
static const Vector camera_pos = {16.0, -24.0, 4.0};
static const Vector camera_at = {0.0, 0.0, 1.4};

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

int anim_frames(void) {
  int frame = 0;

  while (frame * anim_time_step < anim_time_stop) {
    frame++;
  }
  return frame;
}

void anim_scene_create(int shadow_size) {
  Object *floor;
  Object *bottom;

  /* Create the floor. */
  floor =
      sipp_block(FLOORSIZE, FLOORSIZE, 1.0, &floor_surf, floor_shader, WORLD);
  object_move(floor, 0.0, 0.0, -0.5);
  object_add_subobj(sipp_world, floor);

  /* Create the teapot and its bottom. */
  teapot = sipp_teapot(RESOLUTION, &teapot_surf, marble_shader, WORLD);
  bottom = sipp_cylinder(0.375, 0.01, RESOLUTION * 4, &teapot_surf,
                         marble_shader, WORLD);
  object_add_subobj(teapot, bottom);
  object_add_subobj(sipp_world, teapot);

  /*
   * Lit the stage!  A soft spotlight high up on the camera's left
   * throws the teapot's shadow on the floor to the right of it, where
   * the camera sees it even at the top of the jump; a weak directional
   * light fills in the rest.  The teapot moves every frame, so the
   * depth map of the spotlight is rendered anew for every image.
   */
  spotlight_create(-12.0, -7.0, 20.0, 0.0, 0.0, 1.0, 45.0, 1.0, 1.0, 1.0,
                   SPOT_SOFT, TRUE);
  lightsource_create(1.0, 0.0, 0.5, 0.2, 0.2, 0.2, LIGHT_DIRECTION);
  sipp_shadows(TRUE, shadow_size);

  /* Viewing parameters. */
  camera_position(sipp_camera, camera_pos.x, camera_pos.y, camera_pos.z);
  camera_look_at(sipp_camera, camera_at.x, camera_at.y, camera_at.z);
  camera_up(sipp_camera, 0.0, 0.0, 1.0);
  camera_focal(sipp_camera, 0.0625);
}

void anim_scene_default_view(double *azimuth, double *elevation,
                             double *distance) {
  Vector d;

  VecSub(d, camera_pos, camera_at);
  *distance = VecLen(d);
  *azimuth = atan2(d.y, d.x) * 180.0 / M_PI;
  *elevation = asin(d.z / *distance) * 180.0 / M_PI;
}

void anim_scene_view(double azimuth, double elevation, double distance) {
  double az = azimuth * M_PI / 180.0;
  double el = elevation * M_PI / 180.0;

  camera_position(sipp_camera, camera_at.x + distance * cos(el) * cos(az),
                  camera_at.y + distance * cos(el) * sin(az),
                  camera_at.z + distance * sin(el));
  camera_look_at(sipp_camera, camera_at.x, camera_at.y, camera_at.z);
  camera_up(sipp_camera, 0.0, 0.0, 1.0);
}

/*
 * The following code is quite ugly and full of magic numbers.
 * It is basically two parabolas that describe the teapots jump
 * and its "squashing" when it lands.
 */
void anim_scene_place(double time) {
  double t_jump;
  double height;
  double t_scale;
  double xyscaling;
  double zscaling;
  double angle;

  if (time < 0.65) {
    /* During the jump */
    t_jump = time * 1.94464;
    height = 6.2 * t_jump - 9.81 * t_jump * t_jump / 2.0;
    angle = 2.0 * M_PI * time / 0.65;
    xyscaling = 1.0;
    zscaling = 1.0;

  } else {
    /* During the squashing phase. */
    height = 0.0;
    angle = 0.0;
    t_scale = (time - 0.65) * 0.64533;
    zscaling = 1.0 - 6.2 * t_scale + 54.9 * t_scale * t_scale / 2.0;
    xyscaling = 1.0 + (1.0 - zscaling) * M_SQRT1_2;
  }

  /*
   * It is easier to recreate the proper position from scratch
   * for each new frame than to calculate the difference between
   * one image and the next.
   */
  object_clear_transf(teapot);
  object_scale(teapot, xyscaling, xyscaling, zscaling);
  object_move(teapot, 0.0, 0.0, -0.4);
  object_rot_y(teapot, -angle);
  object_move(teapot, 0.0, 0.0, 0.4);
  object_move(teapot, 0.0, 0.0, height);
}
