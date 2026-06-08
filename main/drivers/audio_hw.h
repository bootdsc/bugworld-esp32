#pragma once

#include <stdint.h>
#include <stdbool.h>

bool audio_hw_init(void);
void audio_hw_set_volume(uint8_t vol);
void audio_hw_play(const int16_t* samples, int count);
bool audio_hw_is_playing(void);
