#include "effects.h"
#include "../config.h"
#include <cmath>
#include <cstring>

static Material* s_explosion_mat;
static Material* s_flash_mat;
static Material* s_shield_flare_mat;
static Material* s_debris_mat;

void effects_init(EffectPool* pool) {
    memset(pool, 0, sizeof(EffectPool));

    s_explosion_mat = new Material(0xFD20);
    s_explosion_mat->shadingMode = ShadingMode::UNLIT;
    s_explosion_mat->emissive = true;

    s_flash_mat = new Material(0xFFFF);
    s_flash_mat->shadingMode = ShadingMode::UNLIT;
    s_flash_mat->emissive = true;

    s_shield_flare_mat = new Material(0x07FF);
    s_shield_flare_mat->shadingMode = ShadingMode::UNLIT;
    s_shield_flare_mat->emissive = true;
    s_shield_flare_mat->alpha = 200;

    s_debris_mat = new Material(0x8410);
    s_debris_mat->shadingMode = ShadingMode::UNLIT;
}

static Effect* find_free(EffectPool* pool) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!pool->effects[i].active) return &pool->effects[i];
    }
    return nullptr;
}

void effects_spawn_explosion(EffectPool* pool, float x, float y, float z) {
    Effect* e = find_free(pool);
    if (!e) return;

    e->active = true;
    e->type = FX_EXPLOSION;
    e->x = x;
    e->y = y;
    e->z = z;
    e->timer = 0;
    e->max_timer = 24;
    e->scale = 1.0f;

    if (e->mesh) { delete e->mesh; e->mesh = nullptr; }
    e->mesh = Primitives::createSphere(8, 6, s_explosion_mat);
    if (e->mesh) {
        e->mesh->setPosition((int32_t)x, (int32_t)y, (int32_t)z);
    }
    pool->active_count++;
}

void effects_spawn_muzzle_flash(EffectPool* pool, float x, float y, float z) {
    Effect* e = find_free(pool);
    if (!e) return;

    e->active = true;
    e->type = FX_MUZZLE_FLASH;
    e->x = x;
    e->y = y;
    e->z = z;
    e->timer = 0;
    e->max_timer = 6;
    e->scale = 1.0f;

    if (e->mesh) { delete e->mesh; e->mesh = nullptr; }
    e->mesh = Primitives::createSphere(5, 4, s_flash_mat);
    if (e->mesh) {
        e->mesh->setPosition((int32_t)x, (int32_t)y, (int32_t)z);
    }
    pool->active_count++;
}

void effects_spawn_shield_flare(EffectPool* pool, float x, float y, float z) {
    Effect* e = find_free(pool);
    if (!e) return;

    e->active = true;
    e->type = FX_SHIELD_FLARE;
    e->x = x;
    e->y = y;
    e->z = z;
    e->timer = 0;
    e->max_timer = 12;
    e->scale = 1.0f;

    if (e->mesh) { delete e->mesh; e->mesh = nullptr; }
    e->mesh = Primitives::createSphere(15, 6, s_shield_flare_mat);
    if (e->mesh) {
        e->mesh->setPosition((int32_t)x, (int32_t)y, (int32_t)z);
    }
    pool->active_count++;
}

void effects_spawn_debris(EffectPool* pool, float x, float y, float z) {
    for (int d = 0; d < 3; d++) {
        Effect* e = find_free(pool);
        if (!e) return;

        e->active = true;
        e->type = FX_DEBRIS;
        e->x = x;
        e->y = y;
        e->z = z;
        e->timer = 0;
        e->max_timer = 30;
        e->scale = 1.0f;

        if (e->mesh) { delete e->mesh; e->mesh = nullptr; }
        e->mesh = Primitives::createCube(4, 4, 4, s_debris_mat);
        if (e->mesh) {
            e->mesh->setPosition((int32_t)x, (int32_t)y, (int32_t)z);
        }
        pool->active_count++;
    }
}

void effects_update(EffectPool* pool) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        Effect* e = &pool->effects[i];
        if (!e->active) continue;

        e->timer++;
        float progress = (float)e->timer / e->max_timer;

        switch (e->type) {
            case FX_EXPLOSION:
                e->scale = 1.0f + progress * 3.0f;
                if (e->mesh) {
                    int32_t s = (int32_t)(8.0f * e->scale);
                    e->mesh->scale = {s, s, s};
                }
                break;

            case FX_MUZZLE_FLASH:
                e->scale = 1.0f - progress;
                if (e->mesh) {
                    int32_t s = (int32_t)(5.0f * e->scale);
                    if (s < 1) s = 1;
                    e->mesh->scale = {s, s, s};
                }
                break;

            case FX_SHIELD_FLARE:
                e->scale = 1.0f + progress * 0.5f;
                if (e->mesh) {
                    int32_t s = (int32_t)(15.0f * e->scale);
                    e->mesh->scale = {s, s, s};
                }
                break;

            case FX_DEBRIS:
                e->y -= 2.0f;
                if (e->mesh) {
                    e->mesh->setPosition((int32_t)e->x, (int32_t)e->y, (int32_t)e->z);
                    e->mesh->rotate(10, 15, 5);
                }
                break;

            default:
                break;
        }

        if (e->timer >= e->max_timer) {
            e->active = false;
            if (e->mesh) { delete e->mesh; e->mesh = nullptr; }
            pool->active_count--;
        }
    }

    if (pool->shake_timer > 0) {
        pool->shake_timer--;
    }
}

void effects_trigger_shake(EffectPool* pool, float intensity, int duration) {
    pool->shake_intensity = intensity;
    pool->shake_timer = duration;
}
