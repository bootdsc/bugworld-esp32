#pragma once

#include "enemies.h"
#include "generators.h"

typedef enum {
    LEVEL_1_STRAIGHT_INTO_HELL,
    LEVEL_2_KILL_THEM_ALL,
    LEVEL_COUNT
} LevelID;

typedef struct {
    LevelID id;
    const char* name;
    int wave_count;
    bool has_boss;
    EnemyType boss_type;
    uint16_t sky_color;
} LevelDef;

const LevelDef* level_get_def(LevelID id);
void level_spawn_wave(LevelID id, int wave_index, EnemyPool* pool,
                      float turret_x, float turret_y, float turret_z);
int level_get_wave_count(LevelID id);
