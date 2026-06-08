#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SFX_LASER,
    SFX_EXPLOSION,
    SFX_HIT,
    SFX_SHIELD_HIT,
    SFX_SHIELD_BREAK,
    SFX_BOSS_ROAR,
    SFX_WARP,
    SFX_ENGINE_HUM,
    SFX_MENU_SELECT,
    SFX_MENU_MOVE,
    SFX_RELOAD,
    SFX_WARNING,
    SFX_COUNT
} SoundEffect;

bool audio_init(void);
void audio_play_sfx(SoundEffect sfx);
void audio_set_volume(uint8_t vol);
void audio_set_mute(bool mute);
void audio_update(void);
