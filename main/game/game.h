#pragma once

#include "Jet.hpp"
#include "../drivers/input.h"
#include "turret.h"
#include "enemies.h"
#include "generators.h"
#include "projectiles.h"
#include "effects.h"
#include "levels.h"
#include "cutscene.h"
#include "save.h"
#include "../ui/menus.h"

using namespace Renderer;

typedef enum {
    STATE_TITLE,
    STATE_OPENING_CUTSCENE,
    STATE_LEVEL_INTRO,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_LEVEL_CLEAR,
    STATE_CUTSCENE,
    STATE_WIN,
    STATE_OPTIONS,
    STATE_HIGH_SCORES,
    STATE_SAVE_SELECT,
    STATE_BOSS_APPROACH,
    STATE_OMEGA_DEVICE,
} GameState;

typedef struct {
    GameState state;
    int frame;

    Scene* scene;
    Camera camera;
    DirectionalLight* sun;
    AmbientLight* ambient;

    Turret turret;
    EnemyPool enemies;
    GeneratorArray generators;
    ProjectilePool projectiles;
    EffectPool effects;
    Cutscene cutscene;

    LevelID current_level;
    int current_wave;
    int wave_timer;
    int wave_delay;
    bool boss_spawned;
    int level_intro_timer;
    int game_over_timer;

    int score;
    int difficulty;

    bool sound_enabled;
    bool music_enabled;
    uint8_t volume;
    uint8_t brightness;
    bool pause_pressed_prev;
    bool fire_pressed_prev;

    int high_scores[3];
    int save_slot;
    int pause_touch_item;

    /* Level 1 redesign fields */
    int escape_countdown;      /* Seconds until ship launch */
    bool ship_launched;
    int boss_eyes_remaining;
    bool omega_device_ready;
    int boss_phase;
    int enemies_killed_level;
    int countdown_timer;       /* Frames for countdown display */

    /* Opening cutscene fields */
    int opening_cutscene_timer;
    Object* opening_ship;
    Material* opening_ship_mat;

    /* HUD lock-on */
    int lock_on_timer;
    bool lock_on_active;

    Object* ground_plane;
    Object* starfield[30];
    Material* ground_mat;
    Material* star_mat;

    uint16_t* color_buffer;
    uint16_t* depth_buffer;
} Game;

bool game_init(Game* game);
void game_frame(Game* game, const InputState* input);
void game_cleanup(Game* game);
