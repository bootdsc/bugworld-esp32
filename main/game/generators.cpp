#include "generators.h"
#include "../config.h"
#include <cmath>
#include <cstring>

void generators_init(GeneratorArray* ga) {
    memset(ga, 0, sizeof(GeneratorArray));
}

void generators_create(GeneratorArray* ga, float center_x, float center_y, float center_z, float radius) {
    ga->active_count = MAX_GENERATORS;
    ga->total_health = 0;
    ga->max_total_health = 0;

    for (int i = 0; i < MAX_GENERATORS; i++) {
        Generator* g = &ga->generators[i];
        float angle = (float)i * (2.0f * 3.14159f / MAX_GENERATORS);
        g->x = center_x + cosf(angle) * radius;
        g->y = center_y;
        g->z = center_z + sinf(angle) * radius;
        g->health = 100;
        g->max_health = 100;
        g->active = true;
        g->destroyed = false;
        g->flash_timer = 0;

        g->base_mat = new Material(0x07E0);
        g->base_mat->shadingMode = ShadingMode::UNLIT;

        g->shield_mat = new Material(0x07FF);
        g->shield_mat->shadingMode = ShadingMode::UNLIT;
        g->shield_mat->alpha = 128;
        g->shield_mat->emissive = true;

        g->mesh = Primitives::createCylinder(12, 20, 8, true, g->base_mat);
        g->shield_mesh = Primitives::createSphere(18, 6, g->shield_mat);

        if (g->mesh) {
            g->mesh->setPosition((int32_t)g->x, (int32_t)g->y, (int32_t)g->z);
        }
        if (g->shield_mesh) {
            g->shield_mesh->setPosition((int32_t)g->x, (int32_t)(g->y + 15), (int32_t)g->z);
        }

        ga->total_health += g->health;
        ga->max_total_health += g->max_health;
    }
}

void generators_update(GeneratorArray* ga, int frame) {
    ga->total_health = 0;
    for (int i = 0; i < MAX_GENERATORS; i++) {
        Generator* g = &ga->generators[i];
        if (!g->active) continue;

        ga->total_health += g->health;

        if (g->flash_timer > 0) {
            g->flash_timer--;
        }

        if (g->destroyed) {
            if (g->shield_mesh) {
                g->shield_mesh->enabled = false;
            }
        } else {
            float shield_alpha = 80 + (g->health * 120) / g->max_health;
            if (g->shield_mat) {
                g->shield_mat->alpha = (uint8_t)shield_alpha;
            }
            if (g->shield_mesh) {
                g->shield_mesh->enabled = true;
                float pulse = sinf(frame * 0.1f + i) * 2.0f;
                g->shield_mesh->setPosition((int32_t)g->x, (int32_t)(g->y + 15 + pulse), (int32_t)g->z);
            }
        }

        if (g->mesh) {
            g->mesh->setPosition((int32_t)g->x, (int32_t)g->y, (int32_t)g->z);
        }
    }
}

bool generator_damage(Generator* gen, int amount) {
    if (gen->destroyed) return false;

    gen->health -= amount;
    gen->flash_timer = 10;

    if (gen->health <= 0) {
        gen->health = 0;
        gen->destroyed = true;
        return true;
    }
    return false;
}

void generators_reset(GeneratorArray* ga) {
    for (int i = 0; i < MAX_GENERATORS; i++) {
        Generator* g = &ga->generators[i];
        g->health = g->max_health;
        g->destroyed = false;
        g->flash_timer = 0;
        g->active = true;
        if (g->shield_mesh) g->shield_mesh->enabled = true;
    }
    ga->total_health = ga->max_total_health;
}
