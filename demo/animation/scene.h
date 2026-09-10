/**
 ** scene.h - The somersaulting marble teapot of the animation demo.
 **
 ** The scene is kept apart from the program that renders it, so that the
 ** same animation can be rendered to files (animation.c) or into a
 ** window (../qtanim).  The functions here use the sipp world, camera and
 ** lightsources; call sipp_init() before anim_scene_create().
 **/

#ifndef ANIM_SCENE_H
#define ANIM_SCENE_H

#include <sipp.h>

/*
 * The animation runs from time 0.0 up to (not including) anim_time_stop,
 * one frame every anim_time_step.  It is a closed loop: the frame after
 * the last one is the first one again.
 */
EXTERN const double anim_time_stop;
EXTERN const double anim_time_step;

/* The number of frames in one loop of the animation. */
EXTERN int anim_frames(void);

/*
 * Build the scene: floor, teapot, lights and camera.  The spotlight
 * casts shadows from a depth map of SHADOW_SIZE x SHADOW_SIZE samples,
 * rendered for every frame; about twice the image size looks good.
 */
EXTERN void anim_scene_create(int shadow_size);

/*
 * Put the teapot where it is at TIME (0.0 <= TIME < anim_time_stop).
 * Any time may be asked for, in any order.
 */
EXTERN void anim_scene_place(double time);

/*
 * The camera looks at the point the teapot jumps from, from a position
 * given by an azimuth (degrees around the z axis, counterclockwise from
 * the x axis seen from above), an elevation (degrees above the floor)
 * and a distance.  anim_scene_create() sets up the default view;
 * anim_scene_default_view() tells what it is (without touching the
 * camera), and anim_scene_view() moves the camera to another one.
 */
EXTERN void anim_scene_default_view(double *azimuth, double *elevation,
                                    double *distance);
EXTERN void anim_scene_view(double azimuth, double elevation,
                            double distance);

#endif /* ANIM_SCENE_H */
