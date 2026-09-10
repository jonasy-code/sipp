/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright Equivalent Software HB  1992
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 1, or any later version.
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** You can receive a copy of the GNU General Public License from the
 ** Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

/**
 ** teapot.c - Create a "Utah teapot" as a SIPP object.
 **/

#include <sipp.h>

#include <primitives.h>

static Vector teapot_body[] = {{3.500000E-01, 0.000000E+00, 5.625000E-01},
                               {3.343750E-01, 0.000000E+00, 5.953125E-01},
                               {3.593750E-01, 0.000000E+00, 5.953125E-01},
                               {3.750000E-01, 0.000000E+00, 5.625000E-01},
                               {4.375000E-01, 0.000000E+00, 4.312500E-01},
                               {5.000000E-01, 0.000000E+00, 3.000000E-01},
                               {5.000000E-01, 0.000000E+00, 1.875000E-01},
                               {5.000000E-01, 0.000000E+00, 7.500000E-02},
                               {3.750000E-01, 0.000000E+00, 1.875000E-02},
                               {3.750000E-01, 0.000000E+00, 0.000000E+00}};
static int teapot_body_i[] = {9, 8, 7, 6, 6, 5, 4, 3, 3, 2, 1, 0};

static Vector teapot_lid[] = {{0.000000e+00, 0.000000e+00, 7.500000e-01},
                              {2.000000E-01, 0.000000E+00, 7.500000E-01},
                              {0.000000E+00, 0.000000E+00, 6.750000E-01},
                              {5.000000E-02, 0.000000E+00, 6.375000E-01},
                              {1.000000E-01, 0.000000E+00, 6.000000E-01},
                              {3.250000E-01, 0.000000E+00, 6.000000E-01},
                              {3.500000E-01, 0.000000E+00, 5.625000E-01}};
static int teapot_lid_i[] = {6, 5, 4, 3, 3, 2, 1, 0};

static Vector teapot_spout[] = {{4.250000E-01, -0.000000E+00, 3.187500E-01},
                                {6.500000E-01, -0.000000E+00, 3.187500E-01},
                                {5.750000E-01, -0.000000E+00, 4.875000E-01},
                                {6.750000E-01, -0.000000E+00, 5.625000E-01},
                                {7.000000E-01, -0.000000E+00, 5.812500E-01},
                                {7.250000E-01, -0.000000E+00, 5.812500E-01},
                                {7.000000E-01, -0.000000E+00, 5.625000E-01},
                                {4.250000E-01, -1.650000E-01, 3.187500E-01},
                                {6.500000E-01, -1.650000E-01, 3.187500E-01},
                                {5.750000E-01, -6.250000E-02, 4.875000E-01},
                                {6.750000E-01, -6.250000E-02, 5.625000E-01},
                                {7.000000E-01, -6.250000E-02, 5.812500E-01},
                                {7.250000E-01, -3.750000E-02, 5.812500E-01},
                                {7.000000E-01, -3.750000E-02, 5.625000E-01},
                                {4.250000E-01, -1.650000E-01, 1.125000E-01},
                                {7.750000E-01, -1.650000E-01, 1.687500E-01},
                                {6.000000E-01, -6.250000E-02, 4.687500E-01},
                                {8.250000E-01, -6.250000E-02, 5.625000E-01},
                                {8.812500E-01, -6.250000E-02, 5.859375E-01},
                                {8.625000E-01, -3.750000E-02, 5.906250E-01},
                                {8.000000E-01, -3.750000E-02, 5.625000E-01},
                                {4.250000E-01, -0.000000E+00, 1.125000E-01},
                                {7.750000E-01, -0.000000E+00, 1.687500E-01},
                                {6.000000E-01, -0.000000E+00, 4.687500E-01},
                                {8.250000E-01, -0.000000E+00, 5.625000E-01},
                                {8.812500E-01, -0.000000E+00, 5.859375E-01},
                                {8.625000E-01, -0.000000E+00, 5.906250E-01},
                                {8.000000E-01, -0.000000E+00, 5.625000E-01},
                                {4.250000E-01, 1.650000E-01, 1.125000E-01},
                                {7.750000E-01, 1.650000E-01, 1.687500E-01},
                                {6.000000E-01, 6.250000E-02, 4.687500E-01},
                                {8.250000E-01, 6.250000E-02, 5.625000E-01},
                                {8.812500E-01, 6.250000E-02, 5.859375E-01},
                                {8.625000E-01, 3.750000E-02, 5.906250E-01},
                                {8.000000E-01, 3.750000E-02, 5.625000E-01},
                                {4.250000E-01, 1.650000E-01, 3.187500E-01},
                                {6.500000E-01, 1.650000E-01, 3.187500E-01},
                                {5.750000E-01, 6.250000E-02, 4.875000E-01},
                                {6.750000E-01, 6.250000E-02, 5.625000E-01},
                                {7.000000E-01, 6.250000E-02, 5.812500E-01},
                                {7.250000E-01, 3.750000E-02, 5.812500E-01},
                                {7.000000E-01, 3.750000E-02, 5.625000E-01}};
static int teapot_spout_i[] = {
    21, 22, 23, 24, 14, 15, 16, 17, 7,  8,  9,  10, 0,  1,  2,  3,
    0,  1,  2,  3,  35, 36, 37, 38, 28, 29, 30, 31, 21, 22, 23, 24,
    24, 25, 26, 27, 17, 18, 19, 20, 10, 11, 12, 13, 3,  4,  5,  6,
    3,  4,  5,  6,  38, 39, 40, 41, 31, 32, 33, 34, 24, 25, 26, 27};

static Vector teapot_handle[] = {{-3.750000E-01, -0.000000E+00, 5.250000E-01},
                                 {-6.250000E-01, -0.000000E+00, 5.250000E-01},
                                 {-7.500000E-01, -0.000000E+00, 5.250000E-01},
                                 {-7.500000E-01, -0.000000E+00, 4.125000E-01},
                                 {-7.500000E-01, -0.000000E+00, 3.000000E-01},
                                 {-6.625000E-01, -0.000000E+00, 1.968750E-01},
                                 {-4.750000E-01, -0.000000E+00, 1.125000E-01},
                                 {-3.750000E-01, 7.500000E-02, 5.250000E-01},
                                 {-6.250000E-01, 7.500000E-02, 5.250000E-01},
                                 {-7.500000E-01, 7.500000E-02, 5.250000E-01},
                                 {-7.500000E-01, 7.500000E-02, 4.125000E-01},
                                 {-7.500000E-01, 7.500000E-02, 3.000000E-01},
                                 {-6.625000E-01, 7.500000E-02, 1.968750E-01},
                                 {-4.750000E-01, 7.500000E-02, 1.125000E-01},
                                 {-4.000000E-01, 7.500000E-02, 4.687500E-01},
                                 {-5.750000E-01, 7.500000E-02, 4.687500E-01},
                                 {-6.750000E-01, 7.500000E-02, 4.687500E-01},
                                 {-6.750000E-01, 7.500000E-02, 4.125000E-01},
                                 {-6.750000E-01, 7.500000E-02, 3.562500E-01},
                                 {-6.250000E-01, 7.500000E-02, 2.437500E-01},
                                 {-5.000000E-01, 7.500000E-02, 1.875000E-01},
                                 {-4.000000E-01, -0.000000E+00, 4.687500E-01},
                                 {-5.750000E-01, -0.000000E+00, 4.687500E-01},
                                 {-6.750000E-01, -0.000000E+00, 4.687500E-01},
                                 {-6.750000E-01, -0.000000E+00, 4.125000E-01},
                                 {-6.750000E-01, -0.000000E+00, 3.562500E-01},
                                 {-6.250000E-01, -0.000000E+00, 2.437500E-01},
                                 {-5.000000E-01, -0.000000E+00, 1.875000E-01},
                                 {-4.000000E-01, -7.500000E-02, 4.687500E-01},
                                 {-5.750000E-01, -7.500000E-02, 4.687500E-01},
                                 {-6.750000E-01, -7.500000E-02, 4.687500E-01},
                                 {-6.750000E-01, -7.500000E-02, 4.125000E-01},
                                 {-6.750000E-01, -7.500000E-02, 3.562500E-01},
                                 {-6.250000E-01, -7.500000E-02, 2.437500E-01},
                                 {-5.000000E-01, -7.500000E-02, 1.875000E-01},
                                 {-3.750000E-01, -7.500000E-02, 5.250000E-01},
                                 {-6.250000E-01, -7.500000E-02, 5.250000E-01},
                                 {-7.500000E-01, -7.500000E-02, 5.250000E-01},
                                 {-7.500000E-01, -7.500000E-02, 4.125000E-01},
                                 {-7.500000E-01, -7.500000E-02, 3.000000E-01},
                                 {-6.625000E-01, -7.500000E-02, 1.968750E-01},
                                 {-4.750000E-01, -7.500000E-02, 1.125000E-01}};
static int teapot_handle_i[] = {
    0,  1,  2,  3,  35, 36, 37, 38, 28, 29, 30, 31, 21, 22, 23, 24,
    21, 22, 23, 24, 14, 15, 16, 17, 7,  8,  9,  10, 0,  1,  2,  3,
    3,  4,  5,  6,  38, 39, 40, 41, 31, 32, 33, 34, 24, 25, 26, 27,
    6,  5,  4,  3,  13, 12, 11, 10, 20, 19, 18, 17, 27, 26, 25, 24};

Object *sipp_teapot_body(int resolution, void *surface, Shader *shader,
                         Texture_type texture) {
  return sipp_bezier_rotcurve(sizeof(teapot_body) / sizeof(Vector), teapot_body,
                              sizeof(teapot_body_i) / sizeof(int) / 4,
                              teapot_body_i, resolution, surface, shader,
                              texture);
}

Object *sipp_teapot_lid(int resolution, void *surface, Shader *shader,
                        Texture_type texture) {
  return sipp_bezier_rotcurve(sizeof(teapot_lid) / sizeof(Vector), teapot_lid,
                              sizeof(teapot_lid_i) / sizeof(int) / 4,
                              teapot_lid_i, resolution, surface, shader,
                              texture);
}

Object *sipp_teapot_handle(int resolution, void *surface, Shader *shader,
                           Texture_type texture) {
  return sipp_bezier_patches(
      sizeof(teapot_handle) / sizeof(Vector), teapot_handle,
      sizeof(teapot_handle_i) / sizeof(int) / 16, teapot_handle_i, resolution,
      surface, shader, texture);
}

Object *sipp_teapot_spout(int resolution, void *surface, Shader *shader,
                          Texture_type texture) {
  return sipp_bezier_patches(
      sizeof(teapot_spout) / sizeof(Vector), teapot_spout,
      sizeof(teapot_spout_i) / sizeof(int) / 16, teapot_spout_i, resolution,
      surface, shader, texture);
}

Object *sipp_teapot(int resolution, void *surface, Shader *shader,
                    Texture_type texture) {
  Object *teapot;

  teapot = object_create();

  object_add_subobj(teapot,
                    sipp_teapot_body(resolution, surface, shader, texture));
  object_add_subobj(teapot,
                    sipp_teapot_lid(resolution, surface, shader, texture));
  object_add_subobj(teapot,
                    sipp_teapot_spout(resolution, surface, shader, texture));
  object_add_subobj(teapot,
                    sipp_teapot_handle(resolution, surface, shader, texture));

  return teapot;
}
