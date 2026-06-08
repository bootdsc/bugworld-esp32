#pragma once

#include "../config.h"
#include "Jet.hpp"
#include <stdint.h>

using namespace Renderer;

typedef struct {
    bool active;
    int health;
    int max_health;
    float x, y, z;
    Object* mesh;
    Object* shield_mesh;
    Material* base_mat;
    Material* shield_mat;
    bool destroyed;
    int flash_timer;
} Generator;

typedef struct {
    Generator generators[MAX_GENERATORS];
    int active_count;
    int total_health;
    int max_total_health;
} GeneratorArray;

void generators_init(GeneratorArray* ga);
void generators_create(GeneratorArray* ga, float center_x, float center_y, float center_z, float radius);
void generators_update(GeneratorArray* ga, int frame);
bool generator_damage(Generator* gen, int amount);
void generators_reset(GeneratorArray* ga);
