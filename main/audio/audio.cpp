#include "audio.h"
#include "../drivers/audio_hw.h"
#include "../config.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include <cmath>
#include <cstring>

static const char* TAG = "AUDIO";
static bool s_initialized = false;
static uint8_t s_volume = 128;
static bool s_muted = false;

static int16_t* s_laser_samples = nullptr;
static int s_laser_len = 0;
static int16_t* s_explosion_samples = nullptr;
static int s_explosion_len = 0;
static int16_t* s_hit_samples = nullptr;
static int s_hit_len = 0;
static int16_t* s_shield_hit_samples = nullptr;
static int s_shield_hit_len = 0;
static int16_t* s_shield_break_samples = nullptr;
static int s_shield_break_len = 0;
static int16_t* s_boss_roar_samples = nullptr;
static int s_boss_roar_len = 0;
static int16_t* s_warp_samples = nullptr;
static int s_warp_len = 0;
static int16_t* s_reload_samples = nullptr;
static int s_reload_len = 0;
static int16_t* s_warning_samples = nullptr;
static int s_warning_len = 0;
static int16_t* s_menu_select_samples = nullptr;
static int s_menu_select_len = 0;
static int16_t* s_menu_move_samples = nullptr;
static int s_menu_move_len = 0;

static uint32_t s_rng_state = 12345;
static int16_t s_noise(void) {
    s_rng_state = s_rng_state * 1103515245 + 12345;
    return (int16_t)((s_rng_state >> 16) & 0xFFFF) - 16384;
}

static void gen_laser(void) {
    s_laser_len = 128;
    s_laser_samples = (int16_t*)heap_caps_malloc(s_laser_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_laser_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = 800.0f - (float)i * 4.7f;
        float env = 1.0f - (float)i / s_laser_len;
        s_laser_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * freq * t) * 20000.0f * env);
    }
}

static void gen_explosion(void) {
    s_explosion_len = 512;
    s_explosion_samples = (int16_t*)heap_caps_malloc(s_explosion_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_explosion_len; i++) {
        float env = 1.0f - (float)i / s_explosion_len;
        env *= env;
        s_explosion_samples[i] = (int16_t)(s_noise() * env);
    }
}

static void gen_hit(void) {
    s_hit_len = 64;
    s_hit_samples = (int16_t*)heap_caps_malloc(s_hit_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_hit_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float env = 1.0f - (float)i / s_hit_len;
        s_hit_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * 200.0f * t) * 18000.0f * env);
    }
}

static void gen_shield_hit(void) {
    s_shield_hit_len = 96;
    s_shield_hit_samples = (int16_t*)heap_caps_malloc(s_shield_hit_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_shield_hit_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = 440.0f + sinf(t * 80.0f) * 100.0f;
        float env = 1.0f - (float)i / s_shield_hit_len;
        s_shield_hit_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * freq * t) * 15000.0f * env);
    }
}

static void gen_shield_break(void) {
    s_shield_break_len = 256;
    s_shield_break_samples = (int16_t*)heap_caps_malloc(s_shield_break_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_shield_break_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = 600.0f - (float)i * 1.5f;
        float env = 1.0f - (float)i / s_shield_break_len;
        float sig = sinf(2.0f * 3.14159f * freq * t) * 0.6f + (float)s_noise() / 16384.0f * 0.4f;
        s_shield_break_samples[i] = (int16_t)(sig * 22000.0f * env);
    }
}

static void gen_boss_roar(void) {
    s_boss_roar_len = 1024;
    s_boss_roar_samples = (int16_t*)heap_caps_malloc(s_boss_roar_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_boss_roar_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = 80.0f + sinf(t * 5.0f) * 30.0f;
        float env = (float)i < 128 ? (float)i / 128.0f : 1.0f - (float)(i - 128) / (s_boss_roar_len - 128);
        float sig = sinf(2.0f * 3.14159f * freq * t) * 0.5f + (float)s_noise() / 16384.0f * 0.5f;
        s_boss_roar_samples[i] = (int16_t)(sig * 25000.0f * env);
    }
}

static void gen_warp(void) {
    s_warp_len = 768;
    s_warp_samples = (int16_t*)heap_caps_malloc(s_warp_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_warp_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = 100.0f + (float)i * 2.0f;
        float env = (float)i < 64 ? (float)i / 64.0f : 1.0f - (float)(i - 64) / (s_warp_len - 64);
        s_warp_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * freq * t) * 18000.0f * env);
    }
}

static void gen_reload(void) {
    s_reload_len = 192;
    s_reload_samples = (int16_t*)heap_caps_malloc(s_reload_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_reload_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = 300.0f + (float)i * 3.0f;
        float env = 1.0f - (float)i / s_reload_len;
        s_reload_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * freq * t) * 12000.0f * env);
    }
}

static void gen_warning(void) {
    s_warning_len = 320;
    s_warning_samples = (int16_t*)heap_caps_malloc(s_warning_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_warning_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = (i % 160 < 80) ? 880.0f : 440.0f;
        float env = 1.0f - (float)i / s_warning_len;
        s_warning_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * freq * t) * 16000.0f * env);
    }
}

static void gen_menu_select(void) {
    s_menu_select_len = 96;
    s_menu_select_samples = (int16_t*)heap_caps_malloc(s_menu_select_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_menu_select_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float freq = 660.0f + (float)i * 5.0f;
        float env = 1.0f - (float)i / s_menu_select_len;
        s_menu_select_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * freq * t) * 14000.0f * env);
    }
}

static void gen_menu_move(void) {
    s_menu_move_len = 48;
    s_menu_move_samples = (int16_t*)heap_caps_malloc(s_menu_move_len * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < s_menu_move_len; i++) {
        float t = (float)i / AUDIO_SAMPLE_RATE;
        float env = 1.0f - (float)i / s_menu_move_len;
        s_menu_move_samples[i] = (int16_t)(sinf(2.0f * 3.14159f * 520.0f * t) * 10000.0f * env);
    }
}

bool audio_init(void) {
    if (!audio_hw_init()) {
        ESP_LOGW(TAG, "Audio HW init failed, continuing without sound");
        return false;
    }

    gen_laser();
    gen_explosion();
    gen_hit();
    gen_shield_hit();
    gen_shield_break();
    gen_boss_roar();
    gen_warp();
    gen_reload();
    gen_warning();
    gen_menu_select();
    gen_menu_move();

    s_initialized = true;
    ESP_LOGI(TAG, "Audio engine init OK (%d SFX)", SFX_COUNT);
    return true;
}

void audio_play_sfx(SoundEffect sfx) {
    if (!s_initialized || s_muted) return;

    int16_t* samples = nullptr;
    int len = 0;

    switch (sfx) {
        case SFX_LASER:         samples = s_laser_samples;         len = s_laser_len; break;
        case SFX_EXPLOSION:     samples = s_explosion_samples;     len = s_explosion_len; break;
        case SFX_HIT:           samples = s_hit_samples;           len = s_hit_len; break;
        case SFX_SHIELD_HIT:    samples = s_shield_hit_samples;    len = s_shield_hit_len; break;
        case SFX_SHIELD_BREAK:  samples = s_shield_break_samples;  len = s_shield_break_len; break;
        case SFX_BOSS_ROAR:     samples = s_boss_roar_samples;     len = s_boss_roar_len; break;
        case SFX_WARP:          samples = s_warp_samples;          len = s_warp_len; break;
        case SFX_RELOAD:        samples = s_reload_samples;        len = s_reload_len; break;
        case SFX_WARNING:       samples = s_warning_samples;       len = s_warning_len; break;
        case SFX_MENU_SELECT:   samples = s_menu_select_samples;   len = s_menu_select_len; break;
        case SFX_MENU_MOVE:     samples = s_menu_move_samples;     len = s_menu_move_len; break;
        default: return;
    }

    if (samples && len > 0) {
        audio_hw_set_volume(s_volume);
        audio_hw_play(samples, len);
    }
}

void audio_set_volume(uint8_t vol) {
    s_volume = vol;
}

void audio_set_mute(bool mute) {
    s_muted = mute;
}

void audio_update(void) {
}
