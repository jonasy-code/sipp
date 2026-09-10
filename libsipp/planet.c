#include <math.h>
#include <stdio.h>

#include <sipp.h>

#include <geometric.h>
#include <noise.h>
#include <shaders.h>

/* A reasonably nice brown color */
static Color land = {0.28125, 0.1875, 0.09375};

/* Oceans are usually blue */
static Color sea = {0.0, 0.0, 1.0};

/* And Clouds are white */
static Color cloud = {1.0, 1.0, 1.0};

/*
 * This was designed to work with a unit sphere.
 *
 * Thanks to Jon Buller       jonb@vector.dallas.tx.us
 *
 * WIDTH is the size of the area the sample covers, in the units of LOC;
 * octaves finer than that are left out (see noise.h).
 */
static double turb(int size, double scale_factor, Vector loc, double width) {
  double cur_scale, result, weight;
  int cur;

  weight = noise_weight(width);
  if (weight <= 0.0) {
    return 0.0;
  }
  result = weight * noise(&loc);
  cur_scale = 1.0;
  cur = 1;
  while (cur < size) {
    cur <<= 1;
    cur_scale = cur_scale * scale_factor;
    loc.x *= 2.0;
    loc.y *= 2.0;
    loc.z *= 2.0;
    weight = noise_weight(width * cur);
    if (weight <= 0.0) {
      break;
    }
    result += weight * noise(&loc) * cur_scale;
  }
  return result;
}

void planet_shader(const Shade_point *sp, Lightsource *lights, void *sd_,
                   Color *color, Color *opacity) {
  Surf_desc surf = *(Surf_desc *)sd_; /* Local copy: the shader
                                         must not write into the
                                         shared description */
  Surf_desc *sd = &surf;
  Vector taps[4];
  Vector tmp;
  Color c;
  double width;
  double amt;
  int ntaps, i;

  noise_init();

  /*
   * Average the texture over a few points along the long axis of the
   * sample, each filtered to its short axis (anisotropic filtering).
   */
  ntaps = shade_texture_taps(sp, 4, taps, &width);
  sd->color.red = sd->color.grn = sd->color.blu = 0.0;
  for (i = 0; i < ntaps; i++) {
    VecCopy(tmp, taps[i]);
    if (turb(430, 0.7, tmp, width) > 0.15)
      c = land;
    else
      c = sea;

    VecScalMul(tmp, 12.0, tmp);
    amt = turb(18, 0.6, tmp, width * 12.0);
    if (amt > -0.25) {
      amt += 0.25;
      c.red += amt * (cloud.red - c.red);
      c.grn += amt * (cloud.grn - c.grn);
      c.blu += amt * (cloud.blu - c.blu);
    }
    sd->color.red += c.red / ntaps;
    sd->color.grn += c.grn / ntaps;
    sd->color.blu += c.blu / ntaps;
  }

  basic_shader(sp, lights, sd, color, opacity);
}
