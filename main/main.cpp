#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "drivers/display.h"
#include "drivers/input.h"
#include "audio/audio.h"
#include "game/game.h"
#include "ui/hud.h"
#include "ui/touch_controls.h"
#include "config.h"

static const char* TAG = "MAIN";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== Escape From BugWorld ===");
    ESP_LOGI(TAG, "[1/6] Initializing NVS...");
    ESP_LOGI(TAG, "[1/6] NVS init...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(TAG, "[1/6] NVS OK");

    ESP_LOGI(TAG, "[2/6] Initializing display...");
    if (!display_init()) {
        ESP_LOGE(TAG, "[2/6] FAILED! Display init failed!");
        return;
    }
    display_set_brightness(200);
    ESP_LOGI(TAG, "[2/6] Display OK");

    ESP_LOGI(TAG, "[3/6] Initializing input + touch...");
    if (!input_init()) {
        ESP_LOGE(TAG, "[3/6] FAILED! Input init failed!");
        return;
    }
    ESP_LOGI(TAG, "[3/6] Input OK");

    ESP_LOGI(TAG, "[4/6] Initializing audio...");
    if (!audio_init()) {
        ESP_LOGW(TAG, "[4/6] Audio init failed (continuing without sound)");
    }
    ESP_LOGI(TAG, "[4/6] Audio OK");

    ESP_LOGI(TAG, "[5/6] Initializing touch controls UI...");
    touch_controls_init();
    ESP_LOGI(TAG, "[5/6] Touch controls OK");

    ESP_LOGI(TAG, "[6/6] Initializing game...");
    Game game;
    if (!game_init(&game)) {
        ESP_LOGE(TAG, "[6/6] FAILED! Game init failed!");
        return;
    }
    ESP_LOGI(TAG, "[6/6] Game OK");

    ESP_LOGI(TAG, "=== All init complete, starting game loop ===");

    InputState input;

    while (true) {
        int64_t frame_start = esp_timer_get_time();

        input_scan(&input);
        game_frame(&game, &input);

        touch_controls_draw(game.color_buffer,
                           input.touch_x, input.touch_y, input.touch_pressed,
                            input.stick_x, input.stick_y, input.fire);

        display_flush_full(game.color_buffer);

        /* Frame limiter: target ~60fps (16.67ms per frame) */
        int64_t frame_end = esp_timer_get_time();
        int64_t elapsed_us = frame_end - frame_start;
        int64_t target_us = 16667;
        if (elapsed_us < target_us) {
            vTaskDelay(pdMS_TO_TICKS((target_us - elapsed_us) / 1000));
        }
    }

    game_cleanup(&game);
}
