#include "audio_hw.h"
#include "../config.h"

#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char* TAG = "AUDIO_HW";

static i2s_chan_handle_t s_tx_chan = nullptr;
static uint8_t s_volume = 128;
static volatile bool s_playing = false;

bool audio_hw_init(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 256;

    esp_err_t err = i2s_new_channel(&chan_cfg, &s_tx_chan, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S channel init failed: %s", esp_err_to_name(err));
        return false;
    }

    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg.sample_rate_hz = AUDIO_SAMPLE_RATE;
    std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    std_cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    std_cfg.gpio_cfg.bclk = (gpio_num_t)AUDIO_BCK;
    std_cfg.gpio_cfg.ws = (gpio_num_t)AUDIO_LRCK;
    std_cfg.gpio_cfg.dout = (gpio_num_t)AUDIO_DIN;
    std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
    std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.ws_inv = false;

    err = i2s_channel_init_std_mode(s_tx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S std init failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2s_channel_enable(s_tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S enable failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Audio HW init OK (I2S %dHz 16-bit mono)", AUDIO_SAMPLE_RATE);
    return true;
}

void audio_hw_set_volume(uint8_t vol) {
    s_volume = vol;
}

void audio_hw_play(const int16_t* samples, int count) {
    if (!s_tx_chan || s_volume == 0) return;

    s_playing = true;

    int16_t* scaled = (int16_t*)heap_caps_malloc(count * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!scaled) {
        s_playing = false;
        return;
    }

    for (int i = 0; i < count; i++) {
        int32_t s = ((int32_t)samples[i] * s_volume) / 255;
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        scaled[i] = (int16_t)s;
    }

    size_t bytes_written = 0;
    i2s_channel_write(s_tx_chan, scaled, count * sizeof(int16_t), &bytes_written, pdMS_TO_TICKS(100));

    heap_caps_free(scaled);
    s_playing = false;
}

bool audio_hw_is_playing(void) {
    return s_playing;
}
