#include "input.h"
#include "../config.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cmath>
#include <cstring>

static const char* TAG = "INPUT";

/* ---------- I2C handles ---------- */
static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;

/* ---------- Shared touch state (touch task -> main loop) ---------- */
static volatile int16_t s_raw_x = 0;
static volatile int16_t s_raw_y = 0;
static volatile bool    s_pressed = false;

/* ---------- CST328 registers ---------- */
#define CST328_ADDR           0x1A
#define CST328_REG_TOUCH_NUM  0xD005
#define CST328_REG_TOUCH_XY   0xD000

/* ---------- Screen zones (raw physical coordinates) ---------- */
/* Control area: bottom 100px */
#define ZONE_BOTTOM_Y   220
/* Joystick: left half (120px wide) */
#define ZONE_JOY_W      120
/* Fire: right 60px */
#define ZONE_FIRE_X     180
/* Pause: top 20px */
#define ZONE_PAUSE_Y    20

/* ---------- Joystick state ---------- */
static bool s_joy_tracking = false;
static int  s_joy_origin_x = 0;
static int  s_joy_origin_y = 0;

/* ================================================================ */
static bool cst328_read(int16_t* x, int16_t* y, bool* pressed)
{
    *pressed = false;
    if (!s_dev) return false;

    uint8_t reg[2];
    uint8_t buf[27] = {0};

    /* Read touch count from 0xD005 (low nibble = finger count) */
    reg[0] = (uint8_t)(CST328_REG_TOUCH_NUM >> 8);
    reg[1] = (uint8_t)(CST328_REG_TOUCH_NUM & 0xFF);
    if (i2c_master_transmit(s_dev, reg, 2, 50) != ESP_OK) return false;
    if (i2c_master_receive(s_dev, &buf[5], 1, 50) != ESP_OK) return false;

    uint8_t cnt = buf[5] & 0x0F;
    if (cnt == 0) return true;

    /* Read full touch data block from 0xD000 (27 bytes) */
    reg[0] = (uint8_t)(CST328_REG_TOUCH_XY >> 8);
    reg[1] = (uint8_t)(CST328_REG_TOUCH_XY & 0xFF);
    if (i2c_master_transmit(s_dev, reg, 2, 50) != ESP_OK) return false;
    if (i2c_master_receive(s_dev, buf, 27, 50) != ESP_OK) return false;

    /* CST328 Point 1 format:
     * 0xD000: Status (0x06 = touch)
     * 0xD001: X[11:4]
     * 0xD002: Y[11:4]
     * 0xD003: X[3:0] (high nibble) | Y[3:0] (low nibble)
     * 0xD004: Pressure
     */
    uint8_t status = buf[0];
    if (status != 0x06) return true;

    *x = (int16_t)((buf[1] << 4) | ((buf[3] & 0xF0) >> 4));
    *y = (int16_t)((buf[2] << 4) | (buf[3] & 0x0F));
    *pressed = true;

    if (*x < 0) *x = 0;
    if (*x > 239) *x = 239;
    if (*y < 0) *y = 0;
    if (*y > 319) *y = 319;

    return true;
}

/* ================================================================ */
static void touch_task(void* arg)
{
    (void)arg;
    while (true) {
        int16_t x = 0, y = 0;
        bool p = false;
        if (cst328_read(&x, &y, &p)) {
            s_raw_x = x;
            s_raw_y = y;
            s_pressed = p;
        } else {
            s_pressed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

/* ================================================================ */
bool input_init(void)
{
    /* GPIO buttons */
    int pins[] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B, BTN_X, BTN_Y};
    for (int i = 0; i < 8; i++) {
        gpio_config_t c = {};
        c.pin_bit_mask = (1ULL << pins[i]);
        c.mode = GPIO_MODE_INPUT;
        c.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&c);
    }

    /* Touch reset */
    gpio_config_t rst = {};
    rst.pin_bit_mask = (1ULL << TOUCH_RST);
    rst.mode = GPIO_MODE_OUTPUT;
    gpio_config(&rst);
    gpio_set_level((gpio_num_t)TOUCH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)TOUCH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* I2C bus */
    i2c_master_bus_config_t bus = {};
    bus.i2c_port = I2C_NUM_0;
    bus.sda_io_num = (gpio_num_t)TOUCH_SDA;
    bus.scl_io_num = (gpio_num_t)TOUCH_SCL;
    bus.clk_source = I2C_CLK_SRC_DEFAULT;
    bus.flags.enable_internal_pullup = true;
    if (i2c_new_master_bus(&bus, &s_bus) != ESP_OK) return false;

    /* I2C device */
    i2c_device_config_t dev = {};
    dev.device_address = CST328_ADDR;
    dev.scl_speed_hz = 100000;
    if (i2c_master_bus_add_device(s_bus, &dev, &s_dev) != ESP_OK) return false;

    /* Touch task on Core 1 */
    xTaskCreatePinnedToCore(touch_task, "touch", 4096, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "Input init OK");
    return true;
}

/* ================================================================ */
static void read_buttons(InputState* s)
{
    s->dpad_up    = !gpio_get_level((gpio_num_t)BTN_UP);
    s->dpad_down  = !gpio_get_level((gpio_num_t)BTN_DOWN);
    s->dpad_left  = !gpio_get_level((gpio_num_t)BTN_LEFT);
    s->dpad_right = !gpio_get_level((gpio_num_t)BTN_RIGHT);
    s->btn_a      = !gpio_get_level((gpio_num_t)BTN_A);
    s->btn_b      = !gpio_get_level((gpio_num_t)BTN_B);
    s->btn_x      = !gpio_get_level((gpio_num_t)BTN_X);
    s->btn_y      = !gpio_get_level((gpio_num_t)BTN_Y);
}

/* ================================================================ */
void input_scan(InputState* s)
{
    memset(s, 0, sizeof(InputState));
    read_buttons(s);

    int x = s_raw_x;
    int y = s_raw_y;
    bool pressed = s_pressed;

    s->touch_active = pressed;
    s->touch_pressed = pressed && (x >= 0 && x <= 239 && y >= 0 && y <= 319);
    s->touch_x = x;
    s->touch_y = y;

    /* Joystick reset on release */
    if (!pressed || !s->touch_pressed) {
        s_joy_tracking = false;
        s->stick_x = 0;
        s->stick_y = 0;
        s->fire = false;
        s->pause = false;
        return;
    }

    /* Pause zone: top of screen */
    if (y < ZONE_PAUSE_Y) {
        s->pause = true;
        s->stick_x = 0;
        s->stick_y = 0;
        s->fire = false;
        return;
    }

    /* Above control area: nothing */
    if (y < ZONE_BOTTOM_Y) {
        s->stick_x = 0;
        s->stick_y = 0;
        s->fire = false;
        return;
    }

    /* ----- Bottom 80px control area ----- */
    if (x < ZONE_JOY_W) {
        /* --- Joystick zone (left 100px) --- */
        if (!s_joy_tracking) {
            s_joy_tracking = true;
            s_joy_origin_x = x;
            s_joy_origin_y = y;
        }
        float dx = (float)(x - s_joy_origin_x);
        float dy = (float)(y - s_joy_origin_y);
        float nx = dx / JOY_RADIUS_X;
        float ny = dy / JOY_RADIUS_Y;
        if (nx > 1.0f) nx = 1.0f;
        if (nx < -1.0f) nx = -1.0f;
        if (ny > 1.0f) ny = 1.0f;
        if (ny < -1.0f) ny = -1.0f;
        if (fabsf(nx) < JOY_DEADZONE) nx = 0;
        if (fabsf(ny) < JOY_DEADZONE) ny = 0;
        s->stick_x = nx;
        s->stick_y = ny;
    } else if (x > ZONE_FIRE_X) {
        /* --- Fire zone (right 40px) --- */
        s->fire = true;
    } else {
        /* Dead zone: nothing */
        s->stick_x = 0;
        s->stick_y = 0;
    }
}

/* ================================================================ */
bool input_any_pressed(const InputState* s)
{
    return s->dpad_up || s->dpad_down || s->dpad_left || s->dpad_right ||
           s->btn_a || s->btn_b || s->btn_x || s->btn_y ||
           s->touch_pressed || s->fire;
}
