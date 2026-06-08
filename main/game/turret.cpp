#include "turret.h"
#include "../config.h"
#include <cmath>
#include <cstring>

void turret_init(Turret* t) {
    memset(t, 0, sizeof(Turret));
    t->pan_angle = 0;
    t->tilt_angle = 0;
    t->target_pan = 0;
    t->target_tilt = 0;
    t->crosshair_x = GAME_AREA_W / 2.0f;
    t->crosshair_y = GAME_AREA_H / 2.0f;
    t->fire_cooldown = 0;
    t->reload_timer = 0;
    t->reloading = false;
    t->cockpit_frame = nullptr;
    t->barrel_left = nullptr;
    t->barrel_right = nullptr;
}

void turret_update(Turret* t, const InputState* input, int frame) {
    float stick_x = input->stick_x;
    float stick_y = input->stick_y;

    if (stick_x == 0 && stick_y == 0) {
        if (input->dpad_left)  stick_x = -1.0f;
        if (input->dpad_right) stick_x =  1.0f;
        if (input->dpad_up)    stick_y = -1.0f;
        if (input->dpad_down)  stick_y =  1.0f;
    }

    t->target_pan += stick_x * TURRET_SPEED;
    t->target_tilt -= stick_y * TURRET_SPEED;

    if (t->target_pan < TURRET_PAN_MIN) t->target_pan = TURRET_PAN_MIN;
    if (t->target_pan > TURRET_PAN_MAX) t->target_pan = TURRET_PAN_MAX;
    if (t->target_tilt < TURRET_TILT_MIN) t->target_tilt = TURRET_TILT_MIN;
    if (t->target_tilt > TURRET_TILT_MAX) t->target_tilt = TURRET_TILT_MAX;

    float lerp = 0.25f;
    t->pan_angle += (t->target_pan - t->pan_angle) * lerp;
    t->tilt_angle += (t->target_tilt - t->tilt_angle) * lerp;

    float norm_pan = t->pan_angle / TURRET_PAN_MAX;
    float norm_tilt = t->tilt_angle / TURRET_TILT_MAX;
    t->crosshair_x = GAME_AREA_W / 2.0f + norm_pan * (GAME_AREA_W / 2.0f - 20);
    t->crosshair_y = GAME_AREA_H / 2.0f - norm_tilt * (GAME_AREA_H / 2.0f - 20);

    if (t->fire_cooldown > 0) t->fire_cooldown--;
    if (t->reload_timer > 0) {
        t->reload_timer--;
        if (t->reload_timer <= 0) {
            t->reloading = false;
        }
    }
}

void turret_fire(Turret* t, float* out_dir_x, float* out_dir_y, float* out_dir_z) {
    float pan_rad = t->pan_angle * 3.14159f / 180.0f;
    float tilt_rad = t->tilt_angle * 3.14159f / 180.0f;

    *out_dir_x = sinf(pan_rad) * cosf(tilt_rad);
    *out_dir_y = sinf(tilt_rad);
    *out_dir_z = -cosf(pan_rad) * cosf(tilt_rad);

    t->fire_cooldown = 8;
    t->reloading = true;
    t->reload_timer = 60;  /* 1 second at 60fps */
}

void turret_get_camera_params(const Turret* t, float* pan, float* tilt) {
    *pan = t->pan_angle;
    *tilt = t->tilt_angle;
}

void turret_reset(Turret* t) {
    t->pan_angle = 0;
    t->tilt_angle = 0;
    t->target_pan = 0;
    t->target_tilt = 0;
    t->crosshair_x = GAME_AREA_W / 2.0f;
    t->crosshair_y = GAME_AREA_H / 2.0f;
    t->fire_cooldown = 0;
    t->reload_timer = 0;
    t->reloading = false;
}
