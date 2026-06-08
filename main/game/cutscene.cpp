#include "cutscene.h"
#include "../config.h"
#include <cmath>
#include <cstring>

void cutscene_init(Cutscene* cs) {
    memset(cs, 0, sizeof(Cutscene));

    cs->ship_mat = new Material(0x8410);
    cs->ship_mat->shadingMode = ShadingMode::UNLIT;

    cs->planet_mat = new Material(0x03E0);
    cs->planet_mat->shadingMode = ShadingMode::UNLIT;

    cs->explosion_mat = new Material(0xFD20);
    cs->explosion_mat->shadingMode = ShadingMode::UNLIT;
    cs->explosion_mat->emissive = true;

    cs->ring_mat = new Material(0xF800);
    cs->ring_mat->shadingMode = ShadingMode::UNLIT;
    cs->ring_mat->emissive = true;
    cs->ring_mat->alpha = 180;
}

void cutscene_start(Cutscene* cs) {
    cs->active = true;
    cs->frame = 0;
    cs->total_frames = 300;
    cs->phase = 0;

    cs->ship = Primitives::createCube(10, 5, 20, cs->ship_mat);
    cs->planet = Primitives::createSphere(80, 8, cs->planet_mat);
    cs->explosion_core = Primitives::createSphere(20, 6, cs->explosion_mat);
    cs->explosion_ring = Primitives::createSphere(40, 6, cs->ring_mat);

    cs->ship_x = 0;
    cs->ship_y = 0;
    cs->ship_z = 0;

    cs->cam_x = 0;
    cs->cam_y = 30;
    cs->cam_z = 200;
    cs->cam_target_x = 0;
    cs->cam_target_y = 0;
    cs->cam_target_z = 0;
}

void cutscene_update(Cutscene* cs, Camera* camera, Scene* scene) {
    if (!cs->active) return;

    cs->frame++;
    float t = (float)cs->frame / cs->total_frames;

    if (t < 0.3f) {
        cs->phase = 0;
        float p = t / 0.3f;
        cs->ship_x = 0;
        cs->ship_y = p * 20;
        cs->ship_z = -p * 200;

        cs->cam_x = 0;
        cs->cam_y = 30 + p * 20;
        cs->cam_z = 200 - p * 100;
        cs->cam_target_x = cs->ship_x;
        cs->cam_target_y = cs->ship_y;
        cs->cam_target_z = cs->ship_z - 50;
    }
    else if (t < 0.6f) {
        cs->phase = 1;
        float p = (t - 0.3f) / 0.3f;
        cs->ship_x = 0;
        cs->ship_y = 20 + p * 30;
        cs->ship_z = -200 - p * 300;

        cs->cam_x = sinf(p * 3.14159f) * 100;
        cs->cam_y = 50 + p * 30;
        cs->cam_z = 100 - p * 200;
        cs->cam_target_x = 0;
        cs->cam_target_y = 0;
        cs->cam_target_z = -300;

        if (cs->explosion_core) {
            int32_t s = (int32_t)(20 + p * 80);
            cs->explosion_core->scale = {s, s, s};
            cs->explosion_core->setPosition(0, 0, -400);
        }
        if (cs->explosion_ring) {
            int32_t s = (int32_t)(40 + p * 120);
            cs->explosion_ring->scale = {s, s, s / 3};
            cs->explosion_ring->setPosition(0, 0, -400);
        }
    }
    else {
        cs->phase = 2;
        float p = (t - 0.6f) / 0.4f;
        cs->ship_x = 0;
        cs->ship_y = 50;
        cs->ship_z = -500 - p * 200;

        cs->cam_x = sinf(p * 3.14159f * 0.5f) * 50;
        cs->cam_y = 80 - p * 30;
        cs->cam_z = -100 + p * 100;
        cs->cam_target_x = cs->ship_x;
        cs->cam_target_y = cs->ship_y;
        cs->cam_target_z = cs->ship_z;

        if (cs->explosion_core) {
            int32_t s = (int32_t)(100 + p * 60);
            cs->explosion_core->scale = {s, s, s};
        }
        if (cs->explosion_ring) {
            int32_t s = (int32_t)(160 + p * 100);
            cs->explosion_ring->scale = {s, s, s / 4};
        }
    }

    camera->setPosition((int32_t)cs->cam_x, (int32_t)cs->cam_y, (int32_t)cs->cam_z);
    camera->lookAt(Vector3{(int32_t)cs->cam_target_x, (int32_t)cs->cam_target_y, (int32_t)cs->cam_target_z});

    auto& objects = scene->getObjects();
    objects.clear();

    if (cs->ship) {
        cs->ship->setPosition((int32_t)cs->ship_x, (int32_t)cs->ship_y, (int32_t)cs->ship_z);
        objects.push_back(cs->ship);
    }
    if (cs->planet) {
        cs->planet->setPosition(0, 0, -400);
        objects.push_back(cs->planet);
    }
    if (cs->explosion_core && cs->phase >= 1) {
        objects.push_back(cs->explosion_core);
    }
    if (cs->explosion_ring && cs->phase >= 1) {
        objects.push_back(cs->explosion_ring);
    }

    if (cs->frame >= cs->total_frames) {
        cs->active = false;
    }
}

bool cutscene_is_done(const Cutscene* cs) {
    return !cs->active;
}

void cutscene_cleanup(Cutscene* cs) {
    if (cs->ship) { delete cs->ship; cs->ship = nullptr; }
    if (cs->planet) { delete cs->planet; cs->planet = nullptr; }
    if (cs->explosion_core) { delete cs->explosion_core; cs->explosion_core = nullptr; }
    if (cs->explosion_ring) { delete cs->explosion_ring; cs->explosion_ring = nullptr; }
    cs->active = false;
}
