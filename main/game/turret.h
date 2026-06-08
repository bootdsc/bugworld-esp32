#pragma once

#include "Jet.hpp"
#include "../drivers/input.h"

using namespace Renderer;

typedef struct {
    float pan_angle;
    float tilt_angle;
    float target_pan;
    float target_tilt;

    float crosshair_x;
    float crosshair_y;

    int fire_cooldown;
    int reload_timer;
    bool reloading;

    Object* cockpit_frame;
    Object* barrel_left;
    Object* barrel_right;
} Turret;

void turret_init(Turret* t);
void turret_update(Turret* t, const InputState* input, int frame);
void turret_fire(Turret* t, float* out_dir_x, float* out_dir_y, float* out_dir_z);
void turret_get_camera_params(const Turret* t, float* pan, float* tilt);
void turret_reset(Turret* t);
