#include "enemies.h"
#include "../config.h"
#include <cmath>
#include <cstring>

static Object::Vertex makeVertex(int32_t px, int32_t py, int32_t pz,
                                  int32_t nx, int32_t ny, int32_t nz,
                                  uint16_t color) {
    Object::Vertex v;
    v.position = {px, py, pz};
    v.normal = {nx, ny, nz};
    v.color = color;
    return v;
}

static Material* s_spider_mat;
static Material* s_beetle_mat;
static Material* s_flyer_mat;
static Material* s_wasp_mat;
static Material* s_boss_mat;
static Material* s_queen_mat;
static Material* s_eye_mat;
static Material* s_wing_mat;

static Object* create_spider_body(Material* mat) {
    Object* obj = new Object();
    obj->addVertex(makeVertex(0, 0, 0, 0, 1024, 0, 0x07E0));
    obj->addVertex(makeVertex(15, -5, 0, 0, 512, 888, 0x07E0));
    obj->addVertex(makeVertex(-15, -5, 0, 0, 512, -888, 0x07E0));
    obj->addVertex(makeVertex(0, -5, 20, 0, 0, 1024, 0x07E0));
    obj->addVertex(makeVertex(0, -5, -20, 0, 0, -1024, 0x07E0));
    obj->addVertex(makeVertex(0, 10, 0, 0, 1024, 0, 0x07E0));

    obj->addTriangle(0, 1, 5, mat);
    obj->addTriangle(0, 5, 2, mat);
    obj->addTriangle(1, 3, 5, mat);
    obj->addTriangle(3, 5, 0, mat);
    obj->addTriangle(2, 5, 4, mat);
    obj->addTriangle(4, 5, 0, mat);
    obj->addTriangle(0, 3, 1, mat);
    obj->addTriangle(0, 2, 4, mat);

    obj->computeFlatNormals();
    obj->calculateBoundingBox();
    return obj;
}

static Object* create_beetle_body(Material* mat) {
    Object* obj = new Object();
    obj->addVertex(makeVertex(0, 0, -25, 0, 512, -888, 0xFFE0));
    obj->addVertex(makeVertex(20, -8, 0, 888, 512, 0, 0xFFE0));
    obj->addVertex(makeVertex(-20, -8, 0, -888, 512, 0, 0xFFE0));
    obj->addVertex(makeVertex(0, -8, 25, 0, 512, 888, 0xFFE0));
    obj->addVertex(makeVertex(0, 12, 0, 0, 1024, 0, 0xFFE0));
    obj->addVertex(makeVertex(15, 5, -15, 700, 700, -700, 0xFFE0));
    obj->addVertex(makeVertex(-15, 5, -15, -700, 700, -700, 0xFFE0));
    obj->addVertex(makeVertex(15, 5, 15, 700, 700, 700, 0xFFE0));
    obj->addVertex(makeVertex(-15, 5, 15, -700, 700, 700, 0xFFE0));

    obj->addTriangle(0, 1, 4, mat);
    obj->addTriangle(0, 4, 2, mat);
    obj->addTriangle(3, 4, 1, mat);
    obj->addTriangle(3, 2, 4, mat);
    obj->addTriangle(0, 5, 1, mat);
    obj->addTriangle(0, 2, 6, mat);
    obj->addTriangle(3, 1, 7, mat);
    obj->addTriangle(3, 8, 2, mat);

    obj->computeFlatNormals();
    obj->calculateBoundingBox();
    return obj;
}

static Object* create_flyer_body(Material* mat) {
    Object* obj = new Object();
    obj->addVertex(makeVertex(0, 0, -12, 0, 512, -888, 0xF81F));
    obj->addVertex(makeVertex(8, -4, 0, 888, 512, 0, 0xF81F));
    obj->addVertex(makeVertex(-8, -4, 0, -888, 512, 0, 0xF81F));
    obj->addVertex(makeVertex(0, -4, 12, 0, 512, 888, 0xF81F));
    obj->addVertex(makeVertex(0, 8, 0, 0, 1024, 0, 0xF81F));

    obj->addTriangle(0, 1, 4, mat);
    obj->addTriangle(0, 4, 2, mat);
    obj->addTriangle(3, 4, 1, mat);
    obj->addTriangle(3, 2, 4, mat);
    obj->addTriangle(0, 2, 1, mat);
    obj->addTriangle(3, 1, 2, mat);

    obj->computeFlatNormals();
    obj->calculateBoundingBox();
    return obj;
}

static Object* create_wing(Material* mat, float size) {
    Object* obj = new Object();
    obj->addVertex(makeVertex(0, 0, 0, 0, 1024, 0, 0xFFFF));
    obj->addVertex(makeVertex((int32_t)size, 0, (int32_t)(-size * 0.5f), 0, 1024, 0, 0xFFFF));
    obj->addVertex(makeVertex((int32_t)(size * 0.7f), 0, (int32_t)(size * 0.3f), 0, 1024, 0, 0xFFFF));
    obj->addTriangle(0, 1, 2, mat);
    obj->addTriangle(0, 2, 1, mat);
    obj->computeFlatNormals();
    obj->calculateBoundingBox();
    return obj;
}

static Object* create_leg(Material* mat, float length) {
    Object* obj = new Object();
    obj->addVertex(makeVertex(0, 0, 0, 0, 1024, 0, 0x8410));
    obj->addVertex(makeVertex((int32_t)(length * 0.5f), (int32_t)(-length * 0.3f), 0, 0, 512, 888, 0x8410));
    obj->addVertex(makeVertex((int32_t)length, 0, 0, 0, 0, 1024, 0x8410));
    obj->addTriangle(0, 1, 2, mat);
    obj->addTriangle(0, 2, 1, mat);
    obj->computeFlatNormals();
    obj->calculateBoundingBox();
    return obj;
}

void enemies_init(EnemyPool* pool) {
    memset(pool, 0, sizeof(EnemyPool));

    s_spider_mat = new Material(0x07E0);
    s_spider_mat->shadingMode = ShadingMode::UNLIT;

    s_beetle_mat = new Material(0xFFE0);
    s_beetle_mat->shadingMode = ShadingMode::UNLIT;

    s_flyer_mat = new Material(0xF81F);
    s_flyer_mat->shadingMode = ShadingMode::UNLIT;

    s_wasp_mat = new Material(0xFD20);
    s_wasp_mat->shadingMode = ShadingMode::UNLIT;

    s_boss_mat = new Material(0xF800);
    s_boss_mat->shadingMode = ShadingMode::UNLIT;

    s_queen_mat = new Material(0x800F);
    s_queen_mat->shadingMode = ShadingMode::UNLIT;

    s_eye_mat = new Material(0x07FF);
    s_eye_mat->shadingMode = ShadingMode::UNLIT;
    s_eye_mat->emissive = true;

    s_wing_mat = new Material(0xBDF7);
    s_wing_mat->shadingMode = ShadingMode::UNLIT;
    s_wing_mat->alpha = 180;
}

Enemy* enemies_spawn(EnemyPool* pool, EnemyType type, float x, float y, float z) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!pool->enemies[i].active) {
            Enemy* e = &pool->enemies[i];
            memset(e, 0, sizeof(Enemy));
            e->active = true;
            e->type = type;
            e->x = x;
            e->y = y;
            e->z = z;
            e->spawn_x = x;
            e->spawn_y = y;
            e->spawn_z = z;
            e->wing_angle = 0;
            e->leg_phase = 0;
            e->target_generator = -1;
            e->latched = false;
            e->latch_timer = 0;

            switch (type) {
                case ENEMY_SPIDER:
                    e->body_mesh = create_spider_body(s_spider_mat);
                    e->leg_front_l = create_leg(s_spider_mat, 20);
                    e->leg_front_r = create_leg(s_spider_mat, 20);
                    e->leg_back_l = create_leg(s_spider_mat, 22);
                    e->leg_back_r = create_leg(s_spider_mat, 22);
                    e->health = 2;
                    e->max_health = 2;
                    e->speed = 120;
                    e->shoot_cooldown = 90;
                    break;

                case ENEMY_BEETLE:
                    e->body_mesh = create_beetle_body(s_beetle_mat);
                    e->wing_left = create_wing(s_wing_mat, 25);
                    e->wing_right = create_wing(s_wing_mat, 25);
                    e->health = 4;
                    e->max_health = 4;
                    e->speed = 80;
                    e->shoot_cooldown = 120;
                    break;

                case ENEMY_FLYER:
                    e->body_mesh = create_flyer_body(s_flyer_mat);
                    e->wing_left = create_wing(s_wing_mat, 18);
                    e->wing_right = create_wing(s_wing_mat, 18);
                    e->health = 1;
                    e->max_health = 1;
                    e->speed = 200;
                    e->shoot_cooldown = 60;
                    break;

                case ENEMY_WASP:
                    e->body_mesh = create_flyer_body(s_wasp_mat);
                    e->wing_left = create_wing(s_wing_mat, 15);
                    e->wing_right = create_wing(s_wing_mat, 15);
                    e->health = 2;
                    e->max_health = 2;
                    e->speed = 250;
                    e->shoot_cooldown = 45;
                    break;

                case ENEMY_BOSS_BEETLE:
                    e->body_mesh = create_beetle_body(s_boss_mat);
                    e->body_mesh->scale = {300, 300, 300};
                    e->wing_left = create_wing(s_wing_mat, 60);
                    e->wing_right = create_wing(s_wing_mat, 60);
                    e->health = 40;
                    e->max_health = 40;
                    e->speed = 40;
                    e->shoot_cooldown = 25;
                    break;

                case ENEMY_QUEEN:
                    e->body_mesh = create_beetle_body(s_queen_mat);
                    e->body_mesh->scale = {500, 500, 500};
                    e->wing_left = create_wing(s_wing_mat, 100);
                    e->wing_right = create_wing(s_wing_mat, 100);
                    e->health = 80;
                    e->max_health = 80;
                    e->speed = 20;
                    e->shoot_cooldown = 20;
                    break;

                default:
                    break;
            }

            e->shoot_timer = e->shoot_cooldown;
            e->ai_timer = 0;
            e->ai_phase = 0;

            if (e->body_mesh) {
                e->body_mesh->setPosition((int32_t)x, (int32_t)y, (int32_t)z);
            }

            pool->active_count++;
            return e;
        }
    }
    return nullptr;
}

void enemies_update(EnemyPool* pool, float turret_x, float turret_y, float turret_z,
                    float pan_angle, float tilt_angle, int frame) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy* e = &pool->enemies[i];
        if (!e->active) continue;

        e->ai_timer++;
        e->wing_angle = sinf(e->ai_timer * 0.3f) * 30.0f;
        e->leg_phase = e->ai_timer * 0.15f;

        float dx = turret_x - e->x;
        float dy = turret_y - e->y;
        float dz = turret_z - e->z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        switch (e->type) {
            case ENEMY_SPIDER: {
                float approach_speed = e->speed * 0.016f;
                if (dist > 50) {
                    e->x += (dx / dist) * approach_speed;
                    e->y += (dy / dist) * approach_speed * 0.3f;
                    e->z += (dz / dist) * approach_speed;
                }
                e->x += sinf(e->ai_timer * 0.05f) * 2.0f;
                if (e->leg_front_l) {
                    float leg_ang = sinf(e->leg_phase) * 20.0f;
                    e->leg_front_l->setRotation(0, 0, leg_ang);
                    e->leg_front_r->setRotation(0, 0, -leg_ang);
                    e->leg_back_l->setRotation(0, 0, -leg_ang);
                    e->leg_back_r->setRotation(0, 0, leg_ang);
                }
                break;
            }

            case ENEMY_BEETLE: {
                e->z += e->speed * 0.016f;
                e->x = e->spawn_x + sinf(e->ai_timer * 0.02f) * 80;
                e->y = e->spawn_y + cosf(e->ai_timer * 0.015f) * 30;
                if (e->wing_left) {
                    e->wing_left->setRotation(e->wing_angle, 0, 0);
                    e->wing_right->setRotation(-e->wing_angle, 0, 0);
                }
                break;
            }

            case ENEMY_FLYER: {
                float approach = e->speed * 0.016f;
                if (dist > 30) {
                    e->x += (dx / dist) * approach;
                    e->y += (dy / dist) * approach;
                    e->z += (dz / dist) * approach;
                }
                e->x += sinf(e->ai_timer * 0.08f) * 3.0f;
                e->y += cosf(e->ai_timer * 0.06f) * 2.0f;
                if (e->wing_left) {
                    e->wing_left->setRotation(e->wing_angle * 1.5f, 0, 0);
                    e->wing_right->setRotation(-e->wing_angle * 1.5f, 0, 0);
                }
                break;
            }

            case ENEMY_WASP: {
                if (e->ai_phase == 0) {
                    e->x += sinf(e->ai_timer * 0.1f) * 4.0f;
                    e->y += cosf(e->ai_timer * 0.08f) * 3.0f;
                    e->z += e->speed * 0.016f;
                    if (e->ai_timer > 60) e->ai_phase = 1;
                } else {
                    float rush = e->speed * 1.5f * 0.016f;
                    if (dist > 20) {
                        e->x += (dx / dist) * rush;
                        e->y += (dy / dist) * rush;
                        e->z += (dz / dist) * rush;
                    }
                }
                if (e->wing_left) {
                    e->wing_left->setRotation(e->wing_angle * 2.0f, 0, 0);
                    e->wing_right->setRotation(-e->wing_angle * 2.0f, 0, 0);
                }
                break;
            }

            case ENEMY_BOSS_BEETLE: {
                e->x = e->spawn_x + sinf(e->ai_timer * 0.015f) * 150;
                e->y = e->spawn_y + cosf(e->ai_timer * 0.01f) * 40;
                if (e->wing_left) {
                    e->wing_left->setRotation(e->wing_angle * 0.5f, 0, 0);
                    e->wing_right->setRotation(-e->wing_angle * 0.5f, 0, 0);
                }
                break;
            }

            case ENEMY_QUEEN: {
                e->x = e->spawn_x + sinf(e->ai_timer * 0.008f) * 200;
                e->y = e->spawn_y + cosf(e->ai_timer * 0.005f) * 60;
                if (e->ai_timer % 180 == 0) {
                    e->ai_phase = (e->ai_phase + 1) % 3;
                }
                if (e->wing_left) {
                    e->wing_left->setRotation(e->wing_angle * 0.3f, 0, 0);
                    e->wing_right->setRotation(-e->wing_angle * 0.3f, 0, 0);
                }
                break;
            }

            default:
                break;
        }

        if (e->shoot_timer > 0) {
            e->shoot_timer--;
        }

        if (e->body_mesh) {
            e->body_mesh->setPosition((int32_t)e->x, (int32_t)e->y, (int32_t)e->z);
        }
        if (e->wing_left) {
            e->wing_left->setPosition((int32_t)(e->x - 15), (int32_t)e->y, (int32_t)e->z);
        }
        if (e->wing_right) {
            e->wing_right->setPosition((int32_t)(e->x + 15), (int32_t)e->y, (int32_t)e->z);
        }
        if (e->leg_front_l) {
            e->leg_front_l->setPosition((int32_t)(e->x - 10), (int32_t)(e->y - 5), (int32_t)(e->z - 8));
            e->leg_front_r->setPosition((int32_t)(e->x + 10), (int32_t)(e->y - 5), (int32_t)(e->z - 8));
            e->leg_back_l->setPosition((int32_t)(e->x - 10), (int32_t)(e->y - 5), (int32_t)(e->z + 8));
            e->leg_back_r->setPosition((int32_t)(e->x + 10), (int32_t)(e->y - 5), (int32_t)(e->z + 8));
        }
    }
}

int enemies_get_active_count(const EnemyPool* pool) {
    return pool->active_count;
}

bool enemy_damage(Enemy* enemy, int damage) {
    enemy->health -= damage;
    if (enemy->health <= 0) {
        enemy->health = 0;
        return true;
    }
    return false;
}

void enemies_update_target_generators(EnemyPool* pool, GeneratorArray* gens, int frame) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy* e = &pool->enemies[i];
        if (!e->active) continue;

        e->ai_timer++;
        e->wing_angle = sinf(e->ai_timer * 0.3f) * 30.0f;
        e->leg_phase = e->ai_timer * 0.15f;

        /* Find nearest active generator */
        float nearest_dist = 999999;
        int target_idx = -1;
        float tx = 0, ty = 0, tz = 0;
        for (int j = 0; j < MAX_GENERATORS; j++) {
            Generator* g = &gens->generators[j];
            if (!g->active || g->destroyed) continue;
            float dx = g->x - e->x;
            float dy = g->y - e->y;
            float dz = g->z - e->z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            if (dist < nearest_dist) {
                nearest_dist = dist;
                target_idx = j;
                tx = g->x; ty = g->y; tz = g->z;
            }
        }

        if (e->latched && e->target_generator >= 0) {
            /* Latched on generator: deal damage */
            e->latch_timer++;
            if (e->latch_timer % 15 == 0) { /* damage every 15 frames (~4x/sec) */
                Generator* g = &gens->generators[e->target_generator];
                if (g->active && !g->destroyed) {
                    generator_damage(g, 1);
                } else {
                    e->latched = false;
                    e->target_generator = -1;
                }
            }
            /* Jiggle while latched */
            e->x += sinf(e->ai_timer * 0.2f) * 0.5f;
            e->y += cosf(e->ai_timer * 0.15f) * 0.5f;
        } else if (target_idx >= 0) {
            /* Move toward target generator */
            float dx = tx - e->x;
            float dy = ty - e->y;
            float dz = tz - e->z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            float speed = e->speed * 0.016f;

            if (dist > 30.0f) {
                e->x += (dx / dist) * speed;
                e->y += (dy / dist) * speed * 0.5f;
                e->z += (dz / dist) * speed;
            } else {
                /* Latch on */
                e->latched = true;
                e->target_generator = target_idx;
                e->latch_timer = 0;
            }
        } else {
            /* No generators left, circle aimlessly */
            e->x += sinf(e->ai_timer * 0.05f) * 2.0f;
            e->z += cosf(e->ai_timer * 0.05f) * 2.0f;
        }

        /* Animate wings/legs */
        if (e->wing_left) {
            e->wing_left->setRotation(e->wing_angle, 0, 0);
            e->wing_right->setRotation(-e->wing_angle, 0, 0);
        }
        if (e->leg_front_l) {
            float leg_ang = sinf(e->leg_phase) * 20.0f;
            e->leg_front_l->setRotation(0, 0, leg_ang);
            e->leg_front_r->setRotation(0, 0, -leg_ang);
            e->leg_back_l->setRotation(0, 0, -leg_ang);
            e->leg_back_r->setRotation(0, 0, leg_ang);
        }

        /* Update mesh positions */
        if (e->body_mesh) {
            e->body_mesh->setPosition((int32_t)e->x, (int32_t)e->y, (int32_t)e->z);
        }
        if (e->wing_left) {
            e->wing_left->setPosition((int32_t)(e->x - 15), (int32_t)e->y, (int32_t)e->z);
        }
        if (e->wing_right) {
            e->wing_right->setPosition((int32_t)(e->x + 15), (int32_t)e->y, (int32_t)e->z);
        }
        if (e->leg_front_l) {
            e->leg_front_l->setPosition((int32_t)(e->x - 10), (int32_t)(e->y - 5), (int32_t)(e->z - 8));
            e->leg_front_r->setPosition((int32_t)(e->x + 10), (int32_t)(e->y - 5), (int32_t)(e->z - 8));
            e->leg_back_l->setPosition((int32_t)(e->x - 10), (int32_t)(e->y - 5), (int32_t)(e->z + 8));
            e->leg_back_r->setPosition((int32_t)(e->x + 10), (int32_t)(e->y - 5), (int32_t)(e->z + 8));
        }
    }
}

void enemies_cleanup(EnemyPool* pool, float player_z) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy* e = &pool->enemies[i];
        if (e->active && (e->z > player_z + 1500 || e->health <= 0)) {
            e->active = false;
            pool->active_count--;
        }
    }
}
