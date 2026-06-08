#include "projectiles.h"
#include "../config.h"
#include <cstring>

static Material* s_player_mat;
static Material* s_enemy_mat;

void projectiles_init(ProjectilePool* pool) {
    memset(pool, 0, sizeof(ProjectilePool));

    s_player_mat = new Material(0x07FF);
    s_player_mat->shadingMode = ShadingMode::UNLIT;
    s_player_mat->emissive = true;

    s_enemy_mat = new Material(0xF800);
    s_enemy_mat->shadingMode = ShadingMode::UNLIT;
    s_enemy_mat->emissive = true;
}

void projectile_fire(ProjectilePool* pool, ProjType type,
                     float x, float y, float z,
                     float vx, float vy, float vz, int damage) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!pool->projectiles[i].active) {
            Projectile* p = &pool->projectiles[i];
            p->active = true;
            p->type = type;
            p->x = x;
            p->y = y;
            p->z = z;
            p->vx = vx;
            p->vy = vy;
            p->vz = vz;
            p->damage = damage;
            p->lifetime = 120;

            if (p->mesh) { delete p->mesh; p->mesh = nullptr; }

            if (type == PROJ_PLAYER) {
                p->mesh = Primitives::createCube(3, 3, 12, s_player_mat);
            } else {
                p->mesh = Primitives::createSphere(4, 4, s_enemy_mat);
            }

            if (p->mesh) {
                p->mesh->setPosition((int32_t)x, (int32_t)y, (int32_t)z);
            }

            pool->active_count++;
            return;
        }
    }
}

void projectiles_update(ProjectilePool* pool) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* p = &pool->projectiles[i];
        if (!p->active) continue;

        p->x += p->vx * 0.016f;
        p->y += p->vy * 0.016f;
        p->z += p->vz * 0.016f;
        p->lifetime--;

        if (p->lifetime <= 0) {
            p->active = false;
            pool->active_count--;
            continue;
        }

        if (p->mesh) {
            p->mesh->setPosition((int32_t)p->x, (int32_t)p->y, (int32_t)p->z);
        }
    }
}

int projectiles_get_active_count(const ProjectilePool* pool) {
    return pool->active_count;
}
