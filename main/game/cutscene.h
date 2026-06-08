#pragma once

#include "Jet.hpp"
#include <stdint.h>

using namespace Renderer;

typedef struct {
    bool active;
    int frame;
    int total_frames;
    int phase;

    Object* ship;
    Object* planet;
    Object* explosion_core;
    Object* explosion_ring;
    Material* ship_mat;
    Material* planet_mat;
    Material* explosion_mat;
    Material* ring_mat;

    float ship_x, ship_y, ship_z;
    float cam_x, cam_y, cam_z;
    float cam_target_x, cam_target_y, cam_target_z;
} Cutscene;

void cutscene_init(Cutscene* cs);
void cutscene_start(Cutscene* cs);
void cutscene_update(Cutscene* cs, Camera* camera, Scene* scene);
bool cutscene_is_done(const Cutscene* cs);
void cutscene_cleanup(Cutscene* cs);
