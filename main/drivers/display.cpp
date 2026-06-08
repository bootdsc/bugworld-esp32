#include "display.h"
#include "../config.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char* TAG = "DISPLAY";

#define PHYS_W 320
#define PHYS_H 240

static uint16_t* s_framebuffer = nullptr;
static spi_device_handle_t s_spi = nullptr;
static uint16_t* s_dma_buf = nullptr;
static size_t s_dma_buf_size = 0;

static inline void dc_command(void) { gpio_set_level((gpio_num_t)DISPLAY_DC, 0); }
static inline void dc_data(void)    { gpio_set_level((gpio_num_t)DISPLAY_DC, 1); }

static void spi_write(const uint8_t* data, int len) {
    if (len == 0) return;
    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(s_spi, &t);
}

static void write_command(uint8_t cmd) {
    dc_command();
    spi_write(&cmd, 1);
}

static void write_data(const uint8_t* data, int len) {
    dc_data();
    spi_write(data, len);
}

static void write_data_byte(uint8_t val) {
    dc_data();
    spi_write(&val, 1);
}

static void set_window(int x1, int y1, int x2, int y2) {
    write_command(0x2A);
    { uint8_t d[] = {(uint8_t)(x1>>8),(uint8_t)(x1&0xFF),(uint8_t)(x2>>8),(uint8_t)(x2&0xFF)}; write_data(d, 4); }
    write_command(0x2B);
    { uint8_t d[] = {(uint8_t)(y1>>8),(uint8_t)(y1&0xFF),(uint8_t)(y2>>8),(uint8_t)(y2&0xFF)}; write_data(d, 4); }
    write_command(0x2C);
}

static void st7789_init_sequence(void) {
    gpio_set_level((gpio_num_t)DISPLAY_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)DISPLAY_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    write_command(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    write_command(0x36);
    write_data_byte(0x40);

    write_command(0x3A);
    write_data_byte(0x55);

    write_command(0xB2);
    { uint8_t d[] = {0x0C, 0x0C, 0x00, 0x33, 0x33}; write_data(d, sizeof(d)); }

    write_command(0xB7);
    write_data_byte(0x35);

    write_command(0xBB);
    write_data_byte(0x19);

    write_command(0xC0);
    write_data_byte(0x2C);

    write_command(0xC2);
    write_data_byte(0x01);

    write_command(0xC3);
    write_data_byte(0x12);

    write_command(0xC4);
    write_data_byte(0x20);

    write_command(0xC6);
    write_data_byte(0x0F);

    write_command(0xD0);
    { uint8_t d[] = {0xA4, 0xA1}; write_data(d, sizeof(d)); }

    write_command(0xE0);
    { uint8_t d[] = {0xD0,0x08,0x0E,0x09,0x09,0x05,0x31,0x33,0x48,0x17,0x14,0x15,0x31,0x34}; write_data(d, sizeof(d)); }

    write_command(0xE1);
    { uint8_t d[] = {0xD0,0x08,0x0E,0x09,0x09,0x15,0x31,0x33,0x48,0x17,0x14,0x15,0x31,0x34}; write_data(d, sizeof(d)); }

    write_command(0x21);

    write_command(0x29);
    vTaskDelay(pdMS_TO_TICKS(50));
}

bool display_init(void) {
    ESP_LOGI(TAG, "Init ST7789 raw SPI, MOSI=%d SCLK=%d CS=%d DC=%d RST=%d BL=%d",
             DISPLAY_MOSI, DISPLAY_SCLK, DISPLAY_CS, DISPLAY_DC, DISPLAY_RST, DISPLAY_BL);

    gpio_config_t io_cfg = {};
    io_cfg.pin_bit_mask = (1ULL << DISPLAY_DC) | (1ULL << DISPLAY_RST) | (1ULL << DISPLAY_BL);
    io_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_cfg);

    gpio_set_level((gpio_num_t)DISPLAY_BL, 1);

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = DISPLAY_MOSI;
    bus_cfg.miso_io_num = -1;
    bus_cfg.sclk_io_num = DISPLAY_SCLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = PHYS_W * 2 * 16;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return false;
    }

    spi_device_interface_config_t dev_cfg = {};
    dev_cfg.clock_speed_hz = DISPLAY_SPI_HZ;
    dev_cfg.mode = 0;
    dev_cfg.spics_io_num = DISPLAY_CS;
    dev_cfg.queue_size = 7;
    dev_cfg.flags = SPI_DEVICE_NO_DUMMY;

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(ret));
        return false;
    }

    s_framebuffer = (uint16_t*)heap_caps_malloc(
        DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!s_framebuffer) {
        ESP_LOGE(TAG, "Framebuffer alloc failed");
        return false;
    }
    memset(s_framebuffer, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));

    s_dma_buf_size = PHYS_W * 2 * sizeof(uint16_t);
    s_dma_buf = (uint16_t*)heap_caps_malloc(s_dma_buf_size, MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
    if (!s_dma_buf) {
        ESP_LOGE(TAG, "DMA buffer alloc failed");
        return false;
    }

    st7789_init_sequence();

    ESP_LOGI(TAG, "Display init OK (240x320 portrait -> 320x240 landscape)");
    return true;
}

void display_set_brightness(uint8_t brightness) {
    static bool ledc_init = false;
    if (!ledc_init) {
        ledc_timer_config_t tc = {};
        tc.speed_mode = LEDC_LOW_SPEED_MODE;
        tc.timer_num = LEDC_TIMER_0;
        tc.duty_resolution = LEDC_TIMER_8_BIT;
        tc.freq_hz = 5000;
        tc.clk_cfg = LEDC_AUTO_CLK;
        ledc_timer_config(&tc);

        ledc_channel_config_t cc = {};
        cc.speed_mode = LEDC_LOW_SPEED_MODE;
        cc.channel = LEDC_CHANNEL_0;
        cc.timer_sel = LEDC_TIMER_0;
        cc.gpio_num = (gpio_num_t)DISPLAY_BL;
        cc.duty = brightness;
        cc.hpoint = 0;
        ledc_channel_config(&cc);
        ledc_init = true;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

uint16_t* display_get_framebuffer(void) {
    return s_framebuffer;
}

void display_flush_full(uint16_t* portrait_buf) {
    set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    dc_data();

    int total_bytes = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
    const uint8_t* src = (const uint8_t*)portrait_buf;

    while (total_bytes > 0) {
        int chunk = (total_bytes > (int)s_dma_buf_size) ? (int)s_dma_buf_size : total_bytes;
        const uint16_t* src16 = (const uint16_t*)src;
        int pixels = chunk / 2;
        for (int i = 0; i < pixels; i++) {
            s_dma_buf[i] = __builtin_bswap16(src16[i]);
        }

        spi_transaction_t t = {};
        t.length = chunk * 8;
        t.tx_buffer = s_dma_buf;
        spi_device_polling_transmit(s_spi, &t);

        src += chunk;
        total_bytes -= chunk;
    }
}
