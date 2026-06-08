#pragma once

#include "../config.h"
#include "Jet.hpp"
#include <stdint.h>

using namespace Renderer;

typedef enum {
    FX_EXPLOSION,
    FX_MUZZLE_FLASH,
    FX_SHIELD_FLARE,
    FX_DEBRIS,
    FX_SPARK,
} EffectType;

typedef struct {
    bool active;
    EffectType type;
    float x, y, z;
    int timer;
    int max_timer;
    float scale;
    Object* mesh;
    Material* mat;
} Effect;

typedef struct {
    Effect effects[MAX_EFFECTS];
    int active_count;
    int shake_timer;
    float shake_intensity;
} EffectPool;

void effects_init(EffectPool* pool);
void effects_spawn_explosion(EffectPool* pool, float x, float y, float z);
void effects_spawn_muzzle_flash(EffectPool* pool, float x, float y, float z);
void effects_spawn_shield_flare(EffectPool* pool, float x, float y, float z);
void effects_spawn_debris(EffectPool* pool, float x, float y, float z);
void effects_update(EffectPool* pool);
void effects_trigger_shake(EffectPool* pool, float intensity, int duration);
