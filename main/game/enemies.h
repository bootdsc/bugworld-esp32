#pragma once

#include "../config.h"
#include "Jet.hpp"
#include "generators.h"
#include <stdint.h>

using namespace Renderer;

typedef enum {
    ENEMY_SPIDER,
    ENEMY_BEETLE,
    ENEMY_FLYER,
    ENEMY_WASP,
    ENEMY_BOSS_BEETLE,
    ENEMY_QUEEN,
    ENEMY_COUNT
} EnemyType;

typedef struct {
    bool active;
    EnemyType type;

    Object* body_mesh;
    Object* wing_left;
    Object* wing_right;
    Object* leg_front_l;
    Object* leg_front_r;
    Object* leg_back_l;
    Object* leg_back_r;
    Material* body_mat;
    Material* eye_mat;
    Material* wing_mat;

    float x, y, z;
    float vx, vy, vz;
    float speed;

    int health;
    int max_health;

    int shoot_cooldown;
    int shoot_timer;

    int ai_timer;
    int ai_phase;
    float spawn_x, spawn_y, spawn_z;

    float wing_angle;
    float leg_phase;

    int target_generator;
    bool latched;
    int latch_timer;
} Enemy;

typedef struct {
    Enemy enemies[MAX_ENEMIES];
    int active_count;
} EnemyPool;

void enemies_init(EnemyPool* pool);
Enemy* enemies_spawn(EnemyPool* pool, EnemyType type, float x, float y, float z);
void enemies_update(EnemyPool* pool, float turret_x, float turret_y, float turret_z,
                    float pan_angle, float tilt_angle, int frame);
void enemies_update_target_generators(EnemyPool* pool, GeneratorArray* gens, int frame);
int enemies_get_active_count(const EnemyPool* pool);
bool enemy_damage(Enemy* enemy, int damage);
void enemies_cleanup(EnemyPool* pool, float player_z);
