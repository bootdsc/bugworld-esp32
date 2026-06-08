#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../config.h"

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t crc;

    int current_level;
    int score;
    int high_scores[3];
    int generators_health[5];
    int difficulty;

    uint8_t volume;
    uint8_t brightness;
    bool sound_enabled;
} SaveData;

bool save_init(void);
bool save_load(int slot, SaveData* data);
bool save_write(int slot, const SaveData* data);
bool save_exists(int slot);
void save_compute_crc(SaveData* data);
bool save_verify_crc(const SaveData* data);
