#include "levels.h"
#include "../config.h"
#include <cmath>

static const LevelDef s_levels[LEVEL_COUNT] = {
    {LEVEL_1_STRAIGHT_INTO_HELL, "STRAIGHT INTO HELL", 8, true,  ENEMY_BOSS_BEETLE, 0x000A},
    {LEVEL_2_KILL_THEM_ALL,     "KILL THEM ALL",      6, true,  ENEMY_QUEEN,       0x0010},
};

const LevelDef* level_get_def(LevelID id) {
    if (id < 0 || id >= LEVEL_COUNT) return &s_levels[0];
    return &s_levels[id];
}

int level_get_wave_count(LevelID id) {
    if (id < 0 || id >= LEVEL_COUNT) return 0;
    return s_levels[id].wave_count;
}

void level_spawn_wave(LevelID id, int wave_index, EnemyPool* pool,
                      float turret_x, float turret_y, float turret_z) {
    float spawn_dist = 600.0f;
    float spawn_z = turret_z - spawn_dist;

    switch (id) {
        case LEVEL_1_STRAIGHT_INTO_HELL: {
            switch (wave_index) {
                case 0: {
                    for (int i = 0; i < 3; i++) {
                        float x = -80.0f + i * 80.0f;
                        enemies_spawn(pool, ENEMY_SPIDER, x, 30, spawn_z);
                    }
                    break;
                }
                case 1: {
                    for (int i = 0; i < 2; i++) {
                        float x = -60.0f + i * 120.0f;
                        enemies_spawn(pool, ENEMY_FLYER, x, 60, spawn_z);
                    }
                    enemies_spawn(pool, ENEMY_SPIDER, 0, 20, spawn_z - 50);
                    break;
                }
                case 2: {
                    enemies_spawn(pool, ENEMY_BEETLE, -100, 40, spawn_z);
                    enemies_spawn(pool, ENEMY_BEETLE, 100, 40, spawn_z);
                    for (int i = 0; i < 2; i++) {
                        enemies_spawn(pool, ENEMY_FLYER, -40 + i * 80, 70, spawn_z - 30);
                    }
                    break;
                }
                case 3: {
                    for (int i = 0; i < 4; i++) {
                        float angle = (float)i * 1.57f;
                        float x = sinf(angle) * 120.0f;
                        float z_off = cosf(angle) * 80.0f;
                        enemies_spawn(pool, ENEMY_WASP, x, 50, spawn_z + z_off);
                    }
                    break;
                }
                case 4: {
                    enemies_spawn(pool, ENEMY_BEETLE, 0, 50, spawn_z);
                    for (int i = 0; i < 3; i++) {
                        enemies_spawn(pool, ENEMY_SPIDER, -80 + i * 80, 20, spawn_z - 40);
                    }
                    enemies_spawn(pool, ENEMY_FLYER, -60, 80, spawn_z + 20);
                    enemies_spawn(pool, ENEMY_FLYER, 60, 80, spawn_z + 20);
                    break;
                }
                case 5: {
                    for (int i = 0; i < 3; i++) {
                        enemies_spawn(pool, ENEMY_WASP, -100 + i * 100, 60, spawn_z);
                    }
                    enemies_spawn(pool, ENEMY_BEETLE, 0, 30, spawn_z - 60);
                    break;
                }
                case 6: {
                    for (int i = 0; i < 2; i++) {
                        enemies_spawn(pool, ENEMY_BEETLE, -80 + i * 160, 40, spawn_z);
                    }
                    for (int i = 0; i < 3; i++) {
                        enemies_spawn(pool, ENEMY_FLYER, -60 + i * 60, 70, spawn_z - 30);
                    }
                    for (int i = 0; i < 2; i++) {
                        enemies_spawn(pool, ENEMY_WASP, -100 + i * 200, 50, spawn_z + 30);
                    }
                    break;
                }
                case 7: {
                    for (int i = 0; i < 5; i++) {
                        float angle = (float)i * 1.26f;
                        float x = sinf(angle) * 100.0f;
                        float z_off = cosf(angle) * 60.0f;
                        enemies_spawn(pool, ENEMY_SPIDER, x, 20, spawn_z + z_off);
                    }
                    enemies_spawn(pool, ENEMY_BEETLE, -120, 50, spawn_z - 20);
                    enemies_spawn(pool, ENEMY_BEETLE, 120, 50, spawn_z - 20);
                    break;
                }
                default:
                    break;
            }
            break;
        }

        case LEVEL_2_KILL_THEM_ALL: {
            switch (wave_index) {
                case 0: {
                    for (int i = 0; i < 4; i++) {
                        enemies_spawn(pool, ENEMY_FLYER, -80 + i * 55, 50, spawn_z);
                    }
                    break;
                }
                case 1: {
                    enemies_spawn(pool, ENEMY_BEETLE, -80, 40, spawn_z);
                    enemies_spawn(pool, ENEMY_BEETLE, 80, 40, spawn_z);
                    for (int i = 0; i < 3; i++) {
                        enemies_spawn(pool, ENEMY_WASP, -60 + i * 60, 70, spawn_z - 40);
                    }
                    break;
                }
                case 2: {
                    for (int i = 0; i < 3; i++) {
                        enemies_spawn(pool, ENEMY_SPIDER, -80 + i * 80, 20, spawn_z);
                    }
                    enemies_spawn(pool, ENEMY_BEETLE, 0, 60, spawn_z - 50);
                    break;
                }
                case 3: {
                    for (int i = 0; i < 4; i++) {
                        float angle = (float)i * 1.57f;
                        enemies_spawn(pool, ENEMY_WASP, sinf(angle) * 100, 50, spawn_z + cosf(angle) * 60);
                    }
                    enemies_spawn(pool, ENEMY_FLYER, 0, 80, spawn_z - 30);
                    break;
                }
                case 4: {
                    for (int i = 0; i < 2; i++) {
                        enemies_spawn(pool, ENEMY_BEETLE, -100 + i * 200, 40, spawn_z);
                    }
                    for (int i = 0; i < 4; i++) {
                        enemies_spawn(pool, ENEMY_FLYER, -60 + i * 40, 60 + (i % 2) * 20, spawn_z - 30);
                    }
                    break;
                }
                case 5: {
                    for (int i = 0; i < 3; i++) {
                        enemies_spawn(pool, ENEMY_WASP, -80 + i * 80, 60, spawn_z);
                    }
                    for (int i = 0; i < 3; i++) {
                        enemies_spawn(pool, ENEMY_SPIDER, -60 + i * 60, 20, spawn_z - 50);
                    }
                    enemies_spawn(pool, ENEMY_BEETLE, 0, 40, spawn_z - 80);
                    break;
                }
                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
}
