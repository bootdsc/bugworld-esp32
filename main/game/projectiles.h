#pragma once

#include "../config.h"
#include "Jet.hpp"
#include <stdint.h>

using namespace Renderer;

typedef enum {
    PROJ_PLAYER,
    PROJ_ENEMY,
} ProjType;

typedef struct {
    bool active;
    ProjType type;
    float x, y, z;
    float vx, vy, vz;
    int damage;
    int lifetime;
    Object* mesh;
} Projectile;

typedef struct {
    Projectile projectiles[MAX_PROJECTILES];
    int active_count;
} ProjectilePool;

void projectiles_init(ProjectilePool* pool);
void projectile_fire(ProjectilePool* pool, ProjType type,
                     float x, float y, float z,
                     float vx, float vy, float vz, int damage);
void projectiles_update(ProjectilePool* pool);
int projectiles_get_active_count(const ProjectilePool* pool);
