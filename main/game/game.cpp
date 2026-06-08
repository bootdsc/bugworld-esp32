#include "game.h"
#include "../ui/hud.h"
#include "../drivers/display.h"
#include "../audio/audio.h"
#include "../config.h"
#include "Jet.hpp"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <cstring>
#include <cmath>
#include <cstdio>

using namespace Renderer;

static const char* TAG = "GAME";

static void game_start_level(Game* game);

bool game_init(Game* game) {
    memset(game, 0, sizeof(Game));
    game->state = STATE_OPENING_CUTSCENE;  /* Start with opening cutscene */
    game->frame = 0;
    game->current_level = LEVEL_1_STRAIGHT_INTO_HELL;
    game->current_wave = 0;
    game->wave_timer = 0;
    game->wave_delay = 120;
    game->boss_spawned = false;
    game->level_intro_timer = 120;    /* Short intro before gameplay */
    game->game_over_timer = 0;
    game->score = 0;
    game->difficulty = 1;
    game->sound_enabled = true;
    game->music_enabled = false;
    game->volume = 180;
    game->brightness = 200;
    game->pause_pressed_prev = false;
    game->fire_pressed_prev = false;
    game->save_slot = 0;
    game->pause_touch_item = -1;
    game->opening_cutscene_timer = 240;  /* 4 seconds opening cutscene */
    game->lock_on_timer = 0;
    game->lock_on_active = false;

    for (int i = 0; i < 3; i++) game->high_scores[i] = 0;

    /* Start level immediately */
    game_start_level(game);

    game->color_buffer = display_get_framebuffer();
    game->depth_buffer = (uint16_t*)heap_caps_malloc(
        DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!game->depth_buffer) {
        ESP_LOGE(TAG, "Depth buffer alloc failed");
        return false;
    }

    game->scene = new Scene(game->color_buffer, game->depth_buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    game->scene->setBackcolor(0x000A);
    game->scene->setClearBuffer(true);

    game->camera.setPosition(0, 30, 0);
    game->camera.setFOV(70.0f, DISPLAY_WIDTH);
    game->camera.nearPlane = 5;
    game->camera.farPlane = 4000;
    game->scene->setCamera(&game->camera);

    game->sun = new DirectionalLight(Vector3{45, 60, 0}, Color{255, 245, 220}, 200);
    game->scene->setDirectionalLight(game->sun);

    game->ambient = new AmbientLight(Color{50, 55, 70});
    game->scene->setAmbientLight(game->ambient);

    turret_init(&game->turret);
    enemies_init(&game->enemies);
    generators_init(&game->generators);
    projectiles_init(&game->projectiles);
    effects_init(&game->effects);
    cutscene_init(&game->cutscene);
    menus_init();
    hud_init();

    game->ground_mat = new Material(0x0120);
    game->ground_mat->shadingMode = ShadingMode::UNLIT;
    game->ground_plane = Primitives::createPlane(2000, 2000, game->ground_mat);
    if (game->ground_plane) {
        game->ground_plane->setPosition(0, -80, -500);
    }

    game->star_mat = new Material(0xFFFF);
    game->star_mat->shadingMode = ShadingMode::UNLIT;
    game->star_mat->emissive = true;
    for (int i = 0; i < 30; i++) {
        game->starfield[i] = Primitives::createCube(2, 2, 2, game->star_mat);
        if (game->starfield[i]) {
            float sx = -400 + (i * 27) % 800;
            float sy = 50 + (i * 37) % 200;
            float sz = -300 - (i * 53) % 1500;
            game->starfield[i]->setPosition((int32_t)sx, (int32_t)sy, (int32_t)sz);
        }
    }

    game->opening_ship_mat = new Material(0x8410);
    game->opening_ship_mat->shadingMode = ShadingMode::UNLIT;
    game->opening_ship = Primitives::createCube(50, 20, 100, game->opening_ship_mat);
    if (game->opening_ship) {
        game->opening_ship->setPosition(0, 20, -400);
    }

    audio_set_volume(game->volume);
    audio_set_mute(!game->sound_enabled);

    save_init();

    ESP_LOGI(TAG, "Game initialized");
    return true;
}

static void game_start_level(Game* game) {
    turret_reset(&game->turret);
    enemies_init(&game->enemies);
    projectiles_init(&game->projectiles);
    effects_init(&game->effects);

    generators_create(&game->generators, 0, -40, -400, 120);

    game->current_wave = 0;
    game->wave_timer = game->wave_delay;
    game->boss_spawned = false;
    game->score = 0;
    game->escape_countdown = 90;  /* 90 seconds to ship launch */
    game->ship_launched = false;
    game->boss_eyes_remaining = 6;
    game->omega_device_ready = false;
    game->boss_phase = 0;
    game->enemies_killed_level = 0;
    game->countdown_timer = 0;
}

static void game_update_camera(Game* game) {
    float pan, tilt;
    turret_get_camera_params(&game->turret, &pan, &tilt);

    float pan_rad = pan * 3.14159f / 180.0f;
    float tilt_rad = tilt * 3.14159f / 180.0f;

    game->camera.setPosition(0, 30, 0);

    float look_x = sinf(pan_rad) * 500;
    float look_y = 30 + sinf(tilt_rad) * 500;
    float look_z = -cosf(pan_rad) * cosf(tilt_rad) * 500;

    game->camera.lookAt(Vector3{(int32_t)look_x, (int32_t)look_y, (int32_t)look_z});

    if (game->effects.shake_timer > 0) {
        float shake_x = ((float)(game->frame % 7) - 3.0f) * game->effects.shake_intensity;
        float shake_y = ((float)(game->frame % 5) - 2.0f) * game->effects.shake_intensity;
        game->camera.translate((int32_t)shake_x, (int32_t)shake_y, 0);
    }
}

static void game_check_collisions(Game* game) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* proj = &game->projectiles.projectiles[i];
        if (!proj->active || proj->type != PROJ_PLAYER) continue;

        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy* enemy = &game->enemies.enemies[j];
            if (!enemy->active) continue;

            float dx = fabsf(proj->x - enemy->x);
            float dy = fabsf(proj->y - enemy->y);
            float dz = fabsf(proj->z - enemy->z);
            float hit_dist = (enemy->type == ENEMY_BOSS_BEETLE || enemy->type == ENEMY_QUEEN) ? 80 : 30;

            if (dx < hit_dist && dy < hit_dist && dz < hit_dist) {
                proj->active = false;
                game->projectiles.active_count--;

                if (enemy->type == ENEMY_BOSS_BEETLE && game->current_level == LEVEL_1_STRAIGHT_INTO_HELL) {
                    /* Boss eye shot: destroy one eye */
                    if (game->boss_eyes_remaining > 0) {
                        game->boss_eyes_remaining--;
                        game->score += 500;
                        effects_spawn_explosion(&game->effects, proj->x, proj->y, proj->z);
                        if (game->sound_enabled) audio_play_sfx(SFX_HIT);
                        effects_trigger_shake(&game->effects, 2.0f, 8);
                    }
                    if (game->boss_eyes_remaining <= 0) {
                        /* All eyes destroyed - boss dies */
                        game->score += 5000;
                        effects_spawn_explosion(&game->effects, enemy->x, enemy->y, enemy->z);
                        if (game->sound_enabled) audio_play_sfx(SFX_EXPLOSION);
                        effects_trigger_shake(&game->effects, 6.0f, 20);
                        enemy->active = false;
                        game->enemies.active_count--;
                    }
                    break;
                }

                if (enemy_damage(enemy, proj->damage)) {
                    int points = 100;
                    switch (enemy->type) {
                        case ENEMY_SPIDER: points = 50; break;
                        case ENEMY_BEETLE: points = 150; break;
                        case ENEMY_FLYER: points = 75; break;
                        case ENEMY_WASP: points = 100; break;
                        case ENEMY_BOSS_BEETLE: points = 5000; break;
                        case ENEMY_QUEEN: points = 10000; break;
                        default: break;
                    }
                    game->score += points;
                    game->enemies_killed_level++;
                    effects_spawn_explosion(&game->effects, enemy->x, enemy->y, enemy->z);
                    if (game->sound_enabled) audio_play_sfx(SFX_EXPLOSION);
                    effects_trigger_shake(&game->effects, 3.0f, 10);
                    enemy->active = false;
                    game->enemies.active_count--;
                } else {
                    effects_spawn_muzzle_flash(&game->effects, proj->x, proj->y, proj->z);
                    if (game->sound_enabled) audio_play_sfx(SFX_HIT);
                }
                break;
            }
        }
    }

    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* proj = &game->projectiles.projectiles[i];
        if (!proj->active || proj->type != PROJ_ENEMY) continue;

        for (int j = 0; j < MAX_GENERATORS; j++) {
            Generator* gen = &game->generators.generators[j];
            if (!gen->active || gen->destroyed) continue;

            float dx = fabsf(proj->x - gen->x);
            float dy = fabsf(proj->y - gen->y);
            float dz = fabsf(proj->z - gen->z);

            if (dx < 25 && dy < 25 && dz < 25) {
                proj->active = false;
                game->projectiles.active_count--;

                if (generator_damage(gen, proj->damage)) {
                    effects_spawn_explosion(&game->effects, gen->x, gen->y, gen->z);
                    effects_spawn_debris(&game->effects, gen->x, gen->y, gen->z);
                    if (game->sound_enabled) audio_play_sfx(SFX_SHIELD_BREAK);
                    effects_trigger_shake(&game->effects, 5.0f, 15);
                } else {
                    effects_spawn_shield_flare(&game->effects, gen->x, gen->y + 15, gen->z);
                    if (game->sound_enabled) audio_play_sfx(SFX_SHIELD_HIT);
                }
                break;
            }
        }
    }
}

static void game_handle_shooting(Game* game, const InputState* input) {
    bool fire_now = input->fire || input->btn_a;

    if (fire_now && game->turret.fire_cooldown <= 0) {
        float dir_x, dir_y, dir_z;
        turret_fire(&game->turret, &dir_x, &dir_y, &dir_z);

        float speed = 800.0f;
        float spawn_x = dir_x * 30;
        float spawn_y = 25 + dir_y * 30;
        float spawn_z = dir_z * 30;

        projectile_fire(&game->projectiles, PROJ_PLAYER,
                        spawn_x - 8, spawn_y, spawn_z,
                        dir_x * speed, dir_y * speed, dir_z * speed, 1);
        projectile_fire(&game->projectiles, PROJ_PLAYER,
                        spawn_x + 8, spawn_y, spawn_z,
                        dir_x * speed, dir_y * speed, dir_z * speed, 1);

        effects_spawn_muzzle_flash(&game->effects, spawn_x, spawn_y, spawn_z);
        if (game->sound_enabled) audio_play_sfx(SFX_LASER);
    }
}

static void game_handle_enemy_shooting(Game* game) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy* e = &game->enemies.enemies[i];
        if (!e->active || e->shoot_timer > 0) continue;

        float dx = -e->x;
        float dy = 30 - e->y;
        float dz = -e->z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist > 0 && dist < 500) {
            float speed = 300.0f;
            projectile_fire(&game->projectiles, PROJ_ENEMY,
                            e->x, e->y, e->z,
                            (dx / dist) * speed, (dy / dist) * speed, (dz / dist) * speed, 1);
        }

        e->shoot_timer = e->shoot_cooldown;
    }
}

static void game_update_waves(Game* game) {
    int total_waves = level_get_wave_count(game->current_level);

    if (game->current_wave < total_waves) {
        if (game->wave_timer > 0) {
            game->wave_timer--;
        } else {
            if (enemies_get_active_count(&game->enemies) < MAX_ENEMIES - 3) {
                level_spawn_wave(game->current_level, game->current_wave,
                                 &game->enemies, 0, 30, 0);
                game->current_wave++;
                game->wave_timer = game->wave_delay;
            }
        }
    } else if (!game->boss_spawned) {
        const LevelDef* lvl = level_get_def(game->current_level);
        if (lvl->has_boss && enemies_get_active_count(&game->enemies) < 3) {
            float boss_z = -500;
            float boss_y = 50;
            enemies_spawn(&game->enemies, lvl->boss_type, 0, boss_y, boss_z);
            game->boss_spawned = true;
            if (game->sound_enabled) audio_play_sfx(SFX_BOSS_ROAR);
            effects_trigger_shake(&game->effects, 6.0f, 20);
        }
    }
}

static bool game_check_level_complete(Game* game) {
    if (!game->boss_spawned) return false;

    bool boss_alive = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy* e = &game->enemies.enemies[i];
        if (e->active && (e->type == ENEMY_BOSS_BEETLE || e->type == ENEMY_QUEEN)) {
            boss_alive = true;
            break;
        }
    }

    if (!boss_alive && game->boss_spawned) {
        int alive_gens = 0;
        for (int i = 0; i < MAX_GENERATORS; i++) {
            if (game->generators.generators[i].active && !game->generators.generators[i].destroyed) {
                alive_gens++;
            }
        }
        return alive_gens > 0;
    }
    return false;
}

static bool game_check_game_over(Game* game) {
    int destroyed = 0;
    for (int i = 0; i < MAX_GENERATORS; i++) {
        if (game->generators.generators[i].destroyed) destroyed++;
    }
    return destroyed >= MAX_GENERATORS;
}

static void game_update_high_scores(Game* game) {
    for (int i = 0; i < 3; i++) {
        if (game->score > game->high_scores[i]) {
            for (int j = 2; j > i; j--) {
                game->high_scores[j] = game->high_scores[j - 1];
            }
            game->high_scores[i] = game->score;
            break;
        }
    }
}

void game_frame(Game* game, const InputState* input) {
    game->frame++;

    bool pause_now = input->pause || input->btn_x;
    bool pause_edge = pause_now && !game->pause_pressed_prev;
    game->pause_pressed_prev = pause_now;

    switch (game->state) {
        case STATE_TITLE:
            menus_update(input);
            {
                bool fire_now = input->btn_a || input->fire;
                bool fire_edge = fire_now && !game->fire_pressed_prev;
                game->fire_pressed_prev = fire_now;
                if (fire_edge) {
                    switch (menus_get_selection()) {
                        case TITLE_NEW_GAME:
                            game->current_level = LEVEL_1_STRAIGHT_INTO_HELL;
                            game_start_level(game);
                            game->state = STATE_LEVEL_INTRO;
                            game->level_intro_timer = 120;
                            if (game->sound_enabled) audio_play_sfx(SFX_MENU_SELECT);
                            break;
                        case TITLE_CONTINUE:
                            if (save_exists(0)) {
                                SaveData sd;
                                if (save_load(0, &sd)) {
                                    game->current_level = (LevelID)sd.current_level;
                                    game->score = sd.score;
                                    game->volume = sd.volume;
                                    game->brightness = sd.brightness;
                                    game->sound_enabled = sd.sound_enabled;
                                    game->difficulty = sd.difficulty;
                                    for (int i = 0; i < 3; i++) game->high_scores[i] = sd.high_scores[i];
                                    game_start_level(game);
                                    game->state = STATE_LEVEL_INTRO;
                                    game->level_intro_timer = 120;
                                }
                            }
                            break;
                        case TITLE_OPTIONS:
                            game->state = STATE_OPTIONS;
                            menus_set_current(MENU_OPTIONS);
                            break;
                        case TITLE_HIGH_SCORES:
                            game->state = STATE_HIGH_SCORES;
                            break;
                    }
                }
            }
            break;

        case STATE_OPENING_CUTSCENE:
            game->opening_cutscene_timer--;
            {
                float t = 1.0f - ((float)game->opening_cutscene_timer / 240.0f);
                float cam_z, cam_y;
                if (t < 0.5f) {
                    float p = t / 0.5f;
                    cam_z = 800 - p * 800;
                    cam_y = 100 - p * 70;
                } else {
                    float p = (t - 0.5f) / 0.5f;
                    cam_z = 0;
                    cam_y = 30 - p * 20;
                }
                game->camera.setPosition(0, (int32_t)cam_y, (int32_t)cam_z);
                game->camera.lookAt(Vector3{0, 30, -500});

                auto& objects = game->scene->getObjects();
                objects.clear();
                if (game->ground_plane) objects.push_back(game->ground_plane);
                for (int i = 0; i < 30; i++) {
                    if (game->starfield[i]) objects.push_back(game->starfield[i]);
                }
                if (game->opening_ship) objects.push_back(game->opening_ship);
                game->scene->render();

                hud_draw_string(game->color_buffer, 40, 100, "APPROACHING", 0xFFE0);
                hud_draw_string(game->color_buffer, 60, 115, "TARGET", 0xFFE0);
                if (t > 0.5f) {
                    hud_draw_string(game->color_buffer, 50, 135, "ENTERING", 0x07E0);
                    hud_draw_string(game->color_buffer, 55, 150, "TURRET", 0x07E0);
                }
            }
            if (game->opening_cutscene_timer <= 0) {
                game->state = STATE_LEVEL_INTRO;
                game->level_intro_timer = 120;
            }
            break;

        case STATE_LEVEL_INTRO:
            game->level_intro_timer--;
            if (game->level_intro_timer <= 0) {
                game->state = STATE_PLAYING;
            }
            break;

        case STATE_PLAYING: {
            turret_update(&game->turret, input, game->frame);
            game_update_camera(game);

            /* Level 1: countdown timer */
            if (game->current_level == LEVEL_1_STRAIGHT_INTO_HELL) {
                game->countdown_timer++;
                if (game->countdown_timer >= 60) { /* 1 second at 60fps */
                    game->countdown_timer = 0;
                    game->escape_countdown--;
                }

                /* Spawn waves */
                game_update_waves(game);
                /* Enemies target generators, not player */
                enemies_update_target_generators(&game->enemies, &game->generators, game->frame);

                /* Boss appears at countdown 0, blocking ship launch */
                if (game->escape_countdown <= 0 && !game->boss_spawned) {
                    int alive_gens = 0;
                    for (int i = 0; i < MAX_GENERATORS; i++) {
                        if (game->generators.generators[i].active && !game->generators.generators[i].destroyed)
                            alive_gens++;
                    }
                    if (alive_gens > 0) {
                        game->boss_spawned = true;
                        /* Spawn boss blocking ship */
                        enemies_spawn(&game->enemies, ENEMY_BOSS_BEETLE, 0, 80, -500);
                        game->state = STATE_BOSS_APPROACH;
                        if (game->sound_enabled) audio_play_sfx(SFX_BOSS_ROAR);
                        effects_trigger_shake(&game->effects, 8.0f, 30);
                    } else {
                        /* All generators destroyed before countdown */
                        game->state = STATE_GAME_OVER;
                        game->game_over_timer = 240;
                        game_update_high_scores(game);
                    }
                }
            } else {
                /* Level 2: original behavior */
                game_update_waves(game);
                enemies_update(&game->enemies, 0, 30, 0,
                               game->turret.pan_angle, game->turret.tilt_angle, game->frame);
            }

            game_handle_shooting(game, input);
            game_handle_enemy_shooting(game);
            projectiles_update(&game->projectiles);
            effects_update(&game->effects);
            generators_update(&game->generators, game->frame);

            game_check_collisions(game);

            if (game->current_level == LEVEL_2_KILL_THEM_ALL) {
                if (game_check_level_complete(game)) {
                    game->state = STATE_WIN;
                    game->game_over_timer = 300;
                    game_update_high_scores(game);
                }
            }

            if (game_check_game_over(game)) {
                game->state = STATE_GAME_OVER;
                game->game_over_timer = 240;
                game_update_high_scores(game);
            }

            if (pause_edge) {
                game->state = STATE_PAUSED;
                menus_set_current(MENU_PAUSE);
            }
            break;
        }

        case STATE_BOSS_APPROACH: {
            turret_update(&game->turret, input, game->frame);
            game_update_camera(game);
            game_handle_shooting(game, input);
            projectiles_update(&game->projectiles);
            effects_update(&game->effects);
            generators_update(&game->generators, game->frame);

            /* Check if boss eyes destroyed → ship launches */
            if (game->boss_eyes_remaining <= 0 && !game->ship_launched) {
                game->ship_launched = true;
                game->omega_device_ready = true;
                effects_trigger_shake(&game->effects, 5.0f, 20);
                if (game->sound_enabled) audio_play_sfx(SFX_EXPLOSION);
            }

            /* Fire to detonate Omega Device after ship launched */
            bool omega_fire = input->fire || input->btn_a;
            if (game->omega_device_ready && game->ship_launched && omega_fire) {
                game->state = STATE_OMEGA_DEVICE;
                game->game_over_timer = 180;
                if (game->sound_enabled) audio_play_sfx(SFX_EXPLOSION);
            }

            game_check_collisions(game);

            if (pause_edge) {
                game->state = STATE_PAUSED;
                menus_set_current(MENU_PAUSE);
            }
            break;
        }

        case STATE_OMEGA_DEVICE: {
            game->game_over_timer--;
            effects_update(&game->effects);
            if (game->game_over_timer <= 0) {
                game->state = STATE_WIN;
                game->game_over_timer = 300;
                game_update_high_scores(game);
            }
            bool skip_now = input->btn_a || input->btn_b || input->fire;
            bool skip_edge = skip_now && !game->fire_pressed_prev;
            game->fire_pressed_prev = skip_now;
            if (skip_edge) {
                game->state = STATE_CUTSCENE;
                cutscene_start(&game->cutscene);
            }
            break;
        }

        case STATE_PAUSED: {
            /* Direct touch menu: no joystick, no fire button */
            static bool s_was_touched = false;
            /* Close menu if touching outside the menu box (x=35..205, y=50..190) */
            if (input->touch_active && (input->touch_x < 35 || input->touch_x > 205 ||
                                         input->touch_y < 50 || input->touch_y > 190)) {
                game->state = STATE_PLAYING;
                s_was_touched = false;
                break;
            }
            if (input->touch_active && input->touch_y >= 50 && input->touch_y <= 190) {
                int item = (input->touch_y - 75) / 24;
                if (item < 0) item = 0;
                if (item >= PAUSE_COUNT) item = PAUSE_COUNT - 1;
                menus_set_selection(item);
                game->pause_touch_item = item;
                s_was_touched = true;
            } else if (!input->touch_active && s_was_touched && game->pause_touch_item >= 0) {
                /* Touch released: select highlighted item */
                s_was_touched = false;
                switch (game->pause_touch_item) {
                    case PAUSE_RESUME:
                        game->state = STATE_PLAYING;
                        break;
                    case PAUSE_SAVE:
                        game->state = STATE_SAVE_SELECT;
                        menus_set_current(MENU_SAVE_SELECT);
                        break;
                    case PAUSE_OPTIONS:
                        game->state = STATE_OPTIONS;
                        menus_set_current(MENU_OPTIONS);
                        break;
                }
                if (game->sound_enabled) audio_play_sfx(SFX_MENU_SELECT);
                game->pause_touch_item = -1;
            } else if (!input->touch_active) {
                s_was_touched = false;
            }
            if (input->btn_b) {
                game->state = STATE_PLAYING;
            }
            break;
        }

        case STATE_CUTSCENE:
            cutscene_update(&game->cutscene, &game->camera, game->scene);
            if (cutscene_is_done(&game->cutscene)) {
                game->current_level = LEVEL_2_KILL_THEM_ALL;
                game_start_level(game);
                game->state = STATE_LEVEL_INTRO;
                game->level_intro_timer = 120;
            }
            break;

        case STATE_GAME_OVER:
            game->game_over_timer--;
            {
                bool any_now = input->fire || input->btn_a || input->btn_b;
                bool any_edge = any_now && !game->fire_pressed_prev;
                game->fire_pressed_prev = any_now;
                if (game->game_over_timer <= 0 || any_edge) {
                    game->state = STATE_TITLE;
                    menus_set_current(MENU_TITLE);
                }
            }
            break;

        case STATE_WIN:
            game->game_over_timer--;
            {
                bool any_now = input->fire || input->btn_a || input->btn_b;
                bool any_edge = any_now && !game->fire_pressed_prev;
                game->fire_pressed_prev = any_now;
                if (game->game_over_timer <= 0 || any_edge) {
                    game->state = STATE_TITLE;
                    menus_set_current(MENU_TITLE);
                }
            }
            break;

        case STATE_LEVEL_CLEAR:
            break;

        case STATE_OPTIONS: {
            static int opt_selected = 0;
            static int opt_repeat = 0;
            if (opt_repeat > 0) opt_repeat--;

            /* Close menu if touching outside the options box (x=20..220, y=35..205) */
            if (input->touch_active && (input->touch_x < 20 || input->touch_x > 220 ||
                                         input->touch_y < 35 || input->touch_y > 205)) {
                game->state = STATE_TITLE;
                menus_set_current(MENU_TITLE);
                break;
            }

            bool up = input->dpad_up || input->stick_y < -0.5f;
            bool down = input->dpad_down || input->stick_y > 0.5f;

            if (up && opt_repeat == 0) { opt_selected = (opt_selected + 3) % 4; opt_repeat = 12; }
            if (down && opt_repeat == 0) { opt_selected = (opt_selected + 1) % 4; opt_repeat = 12; }

            {
                bool fire_now = input->btn_a || input->fire;
                bool fire_edge = fire_now && !game->fire_pressed_prev;
                game->fire_pressed_prev = fire_now;
                if (fire_edge) {
                    switch (opt_selected) {
                        case 0: game->volume = (game->volume + 32) % 256; audio_set_volume(game->volume); break;
                        case 1: game->brightness = (game->brightness + 20 > 250) ? 20 : game->brightness + 20;
                                display_set_brightness(game->brightness); break;
                        case 2: game->sound_enabled = !game->sound_enabled; audio_set_mute(!game->sound_enabled); break;
                        case 3: game->difficulty = (game->difficulty + 1) % 3; break;
                    }
                    opt_repeat = 12;
                }
            }

            if (input->btn_b) {
                game->state = STATE_TITLE;
                menus_set_current(MENU_TITLE);
            }

            menus_draw_options(game->color_buffer, opt_selected, game->volume,
                               game->brightness, game->sound_enabled, game->difficulty);
            break;
        }

        case STATE_HIGH_SCORES:
            {
                /* Close on outside click (box: x=30..210, y=45..195) */
                if (input->touch_active && (input->touch_x < 30 || input->touch_x > 210 ||
                                             input->touch_y < 45 || input->touch_y > 195)) {
                    game->state = STATE_TITLE;
                    menus_set_current(MENU_TITLE);
                    break;
                }
                bool any_now = input_any_pressed(input);
                bool any_edge = any_now && !game->fire_pressed_prev;
                game->fire_pressed_prev = any_now;
                if (any_edge) {
                    game->state = STATE_TITLE;
                    menus_set_current(MENU_TITLE);
                }
            }
            menus_draw_high_scores(game->color_buffer, game->high_scores);
            break;

        case STATE_SAVE_SELECT: {
            static int save_selected = 0;
            static int save_repeat = 0;
            if (save_repeat > 0) save_repeat--;

            /* Close on outside click (box: x=35..205, y=55..185) */
            if (input->touch_active && (input->touch_x < 35 || input->touch_x > 205 ||
                                         input->touch_y < 55 || input->touch_y > 185)) {
                game->state = STATE_PAUSED;
                menus_set_current(MENU_PAUSE);
                break;
            }

            bool up = input->dpad_up || input->stick_y < -0.5f;
            bool down = input->dpad_down || input->stick_y > 0.5f;
            if (up && save_repeat == 0) { save_selected = (save_selected + 2) % 3; save_repeat = 12; }
            if (down && save_repeat == 0) { save_selected = (save_selected + 1) % 3; save_repeat = 12; }

            {
                bool fire_now = input->btn_a || input->fire;
                bool fire_edge = fire_now && !game->fire_pressed_prev;
                game->fire_pressed_prev = fire_now;
                if (fire_edge) {
                    SaveData sd;
                    sd.current_level = game->current_level;
                    sd.score = game->score;
                    sd.difficulty = game->difficulty;
                    sd.volume = game->volume;
                    sd.brightness = game->brightness;
                    sd.sound_enabled = game->sound_enabled;
                    for (int i = 0; i < 3; i++) sd.high_scores[i] = game->high_scores[i];
                    for (int i = 0; i < MAX_GENERATORS; i++) {
                        sd.generators_health[i] = game->generators.generators[i].health;
                    }
                    save_write(save_selected, &sd);
                    game->state = STATE_PAUSED;
                    menus_set_current(MENU_PAUSE);
                    if (game->sound_enabled) audio_play_sfx(SFX_MENU_SELECT);
                }
            }

            if (input->btn_b) {
                game->state = STATE_PAUSED;
                menus_set_current(MENU_PAUSE);
            }

            bool slot_exists[SAVE_MAX_SLOTS];
            for (int i = 0; i < SAVE_MAX_SLOTS; i++) slot_exists[i] = save_exists(i);
            menus_draw_save_select(game->color_buffer, save_selected, slot_exists);
            break;
        }
    }

    if (game->state == STATE_PLAYING || game->state == STATE_PAUSED ||
        game->state == STATE_LEVEL_INTRO) {
        auto& objects = game->scene->getObjects();
        objects.clear();

        if (game->ground_plane) objects.push_back(game->ground_plane);
        for (int i = 0; i < 30; i++) {
            if (game->starfield[i]) objects.push_back(game->starfield[i]);
        }

        for (int i = 0; i < MAX_GENERATORS; i++) {
            Generator* g = &game->generators.generators[i];
            if (g->active && g->mesh) objects.push_back(g->mesh);
            if (g->active && g->shield_mesh && !g->destroyed) objects.push_back(g->shield_mesh);
        }

        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy* e = &game->enemies.enemies[i];
            if (!e->active) continue;
            if (e->body_mesh) objects.push_back(e->body_mesh);
            if (e->wing_left) objects.push_back(e->wing_left);
            if (e->wing_right) objects.push_back(e->wing_right);
            if (e->leg_front_l) objects.push_back(e->leg_front_l);
            if (e->leg_front_r) objects.push_back(e->leg_front_r);
            if (e->leg_back_l) objects.push_back(e->leg_back_l);
            if (e->leg_back_r) objects.push_back(e->leg_back_r);
        }

        for (int i = 0; i < MAX_PROJECTILES; i++) {
            Projectile* p = &game->projectiles.projectiles[i];
            if (p->active && p->mesh) objects.push_back(p->mesh);
        }

        for (int i = 0; i < MAX_EFFECTS; i++) {
            Effect* e = &game->effects.effects[i];
            if (e->active && e->mesh) objects.push_back(e->mesh);
        }

        game->scene->render();

        /* Lock-on detection: check if any enemy is aligned with camera */
        {
            float pan_rad = game->turret.pan_angle * 3.14159f / 180.0f;
            float tilt_rad = game->turret.tilt_angle * 3.14159f / 180.0f;
            float cam_dx = sinf(pan_rad);
            float cam_dy = sinf(tilt_rad);
            float cam_dz = -cosf(pan_rad) * cosf(tilt_rad);
            float best_dot = 0.0f;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                Enemy* e = &game->enemies.enemies[i];
                if (!e->active) continue;
                float dx = e->x - 0;
                float dy = e->y - 30;
                float dz = e->z - 0;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                if (dist < 20.0f) continue;
                dx /= dist; dy /= dist; dz /= dist;
                float dot = dx*cam_dx + dy*cam_dy + dz*cam_dz;
                if (dot > best_dot) best_dot = dot;
            }
            if (best_dot > 0.94f) {
                game->lock_on_active = true;
                game->lock_on_timer++;
            } else {
                game->lock_on_active = false;
                game->lock_on_timer = 0;
            }
        }

        hud_draw_crosshair(game->color_buffer,
                           GAME_AREA_W / 2,
                           GAME_AREA_H / 2,
                           game->lock_on_active ? 0xF800 : 0x07FF);

        if (game->lock_on_active && (game->lock_on_timer % 10) < 5) {
            hud_draw_lock_on(game->color_buffer, GAME_AREA_W / 2, GAME_AREA_H / 2);
        }

        hud_draw_string(game->color_buffer, 4, 4, "SCORE:", 0xFFFF);
        char score_buf[16];
        snprintf(score_buf, sizeof(score_buf), "%d", game->score);
        hud_draw_string(game->color_buffer, 50, 4, score_buf, 0xFFE0);

        /* Level 1: countdown timer */
        if (game->current_level == LEVEL_1_STRAIGHT_INTO_HELL && !game->ship_launched) {
            char timer_buf[16];
            snprintf(timer_buf, sizeof(timer_buf), "%02d", game->escape_countdown);
            hud_draw_string(game->color_buffer, 190, 4, "LAUNCH", 0xFFE0);
            hud_draw_string(game->color_buffer, 210, 14, timer_buf, 0xFFE0);
        }

        /* Per-generator health bars */
        for (int i = 0; i < MAX_GENERATORS; i++) {
            Generator* g = &game->generators.generators[i];
            if (!g->active) continue;
            int y = 26 + i * 8;
            int pct = (g->health * 100) / g->max_health;
            uint16_t col = g->destroyed ? 0xF800 : 0x07E0;
            hud_draw_bar(game->color_buffer, 4, y, 50, 5, pct, 100, col, 0x2104);
        }

        int gen_health_pct = 0;
        if (game->generators.max_total_health > 0) {
            gen_health_pct = (game->generators.total_health * 100) / game->generators.max_total_health;
        }
        hud_draw_string(game->color_buffer, 4, 70, "BASE:", 0xFFFF);
        hud_draw_bar(game->color_buffer, 44, 70, 60, 7, gen_health_pct, 100, 0x07E0, 0x2104);

        /* Boss eye count during boss approach */
        if (game->state == STATE_BOSS_APPROACH) {
            hud_draw_string(game->color_buffer, 60, 210, "EYES", 0xF800);
            char eye_buf[8];
            snprintf(eye_buf, sizeof(eye_buf), "%d", game->boss_eyes_remaining);
            hud_draw_string(game->color_buffer, 95, 210, eye_buf, 0xFFE0);
        }

        /* Boss approach HUD messages */
        if (game->state == STATE_BOSS_APPROACH) {
            if (game->boss_eyes_remaining > 0) {
                /* Still shooting eyes */
            } else if (!game->ship_launched) {
                /* Eyes destroyed, ship launching */
                hud_draw_string(game->color_buffer, 30, 120, "SHIP", 0x07E0);
                hud_draw_string(game->color_buffer, 20, 135, "LAUNCH", 0x07E0);
            } else {
                /* Ship clear, Omega ready */
                hud_draw_string(game->color_buffer, 30, 120, "OMEGA", 0x07E0);
                hud_draw_string(game->color_buffer, 30, 135, "READY", 0x07E0);
                hud_draw_string(game->color_buffer, 20, 150, "FIRE", 0xFFE0);
            }
        }

        if (game->state == STATE_LEVEL_INTRO) {
            const LevelDef* lvl = level_get_def(game->current_level);
            hud_draw_string(game->color_buffer, 50, 100, "STAGE", 0xFFFF);
            hud_draw_string(game->color_buffer, 30, 115, lvl->name, 0xFFE0);
        }
    }

    /* Render for boss approach state */
    if (game->state == STATE_BOSS_APPROACH) {
        auto& objects = game->scene->getObjects();
        objects.clear();
        if (game->ground_plane) objects.push_back(game->ground_plane);
        for (int i = 0; i < 30; i++) {
            if (game->starfield[i]) objects.push_back(game->starfield[i]);
        }
        for (int i = 0; i < MAX_GENERATORS; i++) {
            Generator* g = &game->generators.generators[i];
            if (g->active && g->mesh) objects.push_back(g->mesh);
            if (g->active && g->shield_mesh && !g->destroyed) objects.push_back(g->shield_mesh);
        }
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy* e = &game->enemies.enemies[i];
            if (!e->active) continue;
            if (e->body_mesh) objects.push_back(e->body_mesh);
            if (e->wing_left) objects.push_back(e->wing_left);
            if (e->wing_right) objects.push_back(e->wing_right);
        }
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            Projectile* p = &game->projectiles.projectiles[i];
            if (p->active && p->mesh) objects.push_back(p->mesh);
        }
        for (int i = 0; i < MAX_EFFECTS; i++) {
            Effect* e = &game->effects.effects[i];
            if (e->active && e->mesh) objects.push_back(e->mesh);
        }
        game->scene->render();
        hud_draw_crosshair(game->color_buffer, GAME_AREA_W / 2, GAME_AREA_H / 2, 0xF800);
        hud_draw_string(game->color_buffer, 4, 4, "SCORE:", 0xFFFF);
        char score_buf[16];
        snprintf(score_buf, sizeof(score_buf), "%d", game->score);
        hud_draw_string(game->color_buffer, 50, 4, score_buf, 0xFFE0);
        hud_draw_string(game->color_buffer, 60, 210, "EYES", 0xF800);
        char eye_buf[8];
        snprintf(eye_buf, sizeof(eye_buf), "%d", game->boss_eyes_remaining);
        hud_draw_string(game->color_buffer, 95, 210, eye_buf, 0xFFE0);
        if (game->boss_eyes_remaining > 0) {
            /* Shooting eyes */
        } else if (!game->ship_launched) {
            hud_draw_string(game->color_buffer, 30, 120, "SHIP", 0x07E0);
            hud_draw_string(game->color_buffer, 20, 135, "LAUNCH", 0x07E0);
        } else {
            hud_draw_string(game->color_buffer, 30, 120, "OMEGA", 0x07E0);
            hud_draw_string(game->color_buffer, 30, 135, "READY", 0x07E0);
            hud_draw_string(game->color_buffer, 20, 150, "FIRE", 0xFFE0);
        }
    }

    /* Omega Device screen */
    if (game->state == STATE_OMEGA_DEVICE) {
        for (int y = 0; y < GAME_AREA_H; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                game->color_buffer[y * DISPLAY_WIDTH + x] = 0x07E0; /* Green flash */
            }
        }
        hud_draw_string(game->color_buffer, 60, 100, "OMEGA", 0xFFFF);
        hud_draw_string(game->color_buffer, 50, 115, "DEVICE", 0xFFFF);
        hud_draw_string(game->color_buffer, 40, 130, "DETONATED", 0xFFFF);
        hud_draw_string(game->color_buffer, 30, 160, "SYSTEM", 0xFFE0);
        hud_draw_string(game->color_buffer, 30, 175, "PURGED", 0xFFE0);
    }

    if (game->state == STATE_TITLE) {
        game->scene->setBackcolor(0x000A);
        auto& objects = game->scene->getObjects();
        objects.clear();
        for (int i = 0; i < 30; i++) {
            if (game->starfield[i]) objects.push_back(game->starfield[i]);
        }
        game->scene->render();
        menus_draw_title(game->color_buffer, menus_get_selection());
    }

    if (game->state == STATE_PAUSED) {
        menus_draw_pause(game->color_buffer, menus_get_selection());
    }

    if (game->state == STATE_GAME_OVER) {
        menus_draw_game_over(game->color_buffer, game->score);
    }

    if (game->state == STATE_WIN) {
        menus_draw_win(game->color_buffer, game->score);
    }

    if (game->state == STATE_CUTSCENE) {
        game->scene->setBackcolor(0x0000);
        game->scene->render();
    }
}

void game_cleanup(Game* game) {
    cutscene_cleanup(&game->cutscene);
    if (game->scene) delete game->scene;
    if (game->depth_buffer) heap_caps_free(game->depth_buffer);
}
