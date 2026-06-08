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

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;

static volatile int16_t s_raw_x = -1;
static volatile int16_t s_raw_y = -1;
static volatile bool    s_pressed = false;

#define CST328_ADDR           0x1A
#define CST328_REG_TOUCH_NUM  0xD005
#define CST328_REG_TOUCH_XY   0xD000

#define ZONE_BOTTOM_Y   220
#define ZONE_JOY_W      150
#define ZONE_FIRE_X     160
#define ZONE_PAUSE_Y    20

static bool s_joy_tracking = false;
static int  s_joy_origin_x = 0;
static int  s_joy_origin_y = 0;

static bool cst328_read(int16_t* x, int16_t* y, bool* pressed)
{
    *pressed = false;
    if (!s_dev) return false;

    uint8_t reg[2];
    uint8_t buf[27] = {0};

    // Read finger count
    reg[0] = 0xD0; reg[1] = 0x05;
    i2c_master_transmit(s_dev, reg, 2, 20);
    i2c_master_receive(s_dev, buf, 1, 20);

    if ((buf[0] & 0x0F) == 0) return true;

    // Read touch data
    reg[0] = 0xD0; reg[1] = 0x00;
    i2c_master_transmit(s_dev, reg, 2, 20);
    i2c_master_receive(s_dev, buf, 27, 20);

    if (buf[0] != 0x06) return true;   // Not a valid press

    *x = (int16_t)((buf[1] << 4) | ((buf[3] & 0xF0) >> 4));
    *y = (int16_t)((buf[2] << 4) | (buf[3] & 0x0F));

    *pressed = (*x >= 0 && *x <= 239 && *y >= 0 && *y <= 319);
    return true;
}

static void touch_task(void* arg)
{
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

bool input_init(void)
{
    // Buttons
    int pins[] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B, BTN_X, BTN_Y};
    for (int i = 0; i < 8; i++) {
        gpio_config_t c = {};
        c.pin_bit_mask = 1ULL << pins[i];
        c.mode = GPIO_MODE_INPUT;
        c.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&c);
    }

    // Touch RST
    gpio_set_direction((gpio_num_t)TOUCH_RST, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)TOUCH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)TOUCH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // I2C
    i2c_master_bus_config_t bus = {};
    bus.i2c_port = I2C_NUM_0;
    bus.sda_io_num = (gpio_num_t)TOUCH_SDA;
    bus.scl_io_num = (gpio_num_t)TOUCH_SCL;
    bus.clk_source = I2C_CLK_SRC_DEFAULT;
    bus.flags.enable_internal_pullup = true;
    i2c_new_master_bus(&bus, &s_bus);

    i2c_device_config_t dev = {};
    dev.device_address = CST328_ADDR;
    dev.scl_speed_hz = 100000;
    i2c_master_bus_add_device(s_bus, &dev, &s_dev);

    xTaskCreatePinnedToCore(touch_task, "touch", 4096, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "Input + CST328 OK");
    return true;
}

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

void input_scan(InputState* s)
{
    memset(s, 0, sizeof(InputState));
    read_buttons(s);

    int x = s_raw_x;
    int y = s_raw_y;
    bool pressed = s_pressed;

    s->touch_active = pressed;
    s->touch_pressed = pressed;
    s->touch_x = x;
    s->touch_y = y;

    if (!pressed) {
        s_joy_tracking = false;
        s->stick_x = s->stick_y = 0;
        s->fire = s->pause = false;
        return;
    }

    if (y < ZONE_PAUSE_Y) { s->pause = true; return; }
    if (y < ZONE_BOTTOM_Y) return;   // game area

    if (x < ZONE_JOY_W) {
        if (!s_joy_tracking) {
            s_joy_tracking = true;
            s_joy_origin_x = x; s_joy_origin_y = y;
        }
        float dx = x - s_joy_origin_x;
        float dy = y - s_joy_origin_y;
        float nx = dx / JOY_RADIUS_X;
        float ny = dy / JOY_RADIUS_Y;
        if (nx > 1) nx = 1;
        if (nx < -1) nx = -1;
        if (ny > 1) ny = 1;
        if (ny < -1) ny = -1;
        if (fabsf(nx) < JOY_DEADZONE) nx = 0;
        if (fabsf(ny) < JOY_DEADZONE) ny = 0;
        s->stick_x = nx;
        s->stick_y = ny;
    } else if (x > ZONE_FIRE_X) {
        s->fire = true;
    }
}

bool input_any_pressed(const InputState* s)
{
    return s->dpad_up || s->dpad_down || s->dpad_left || s->dpad_right ||
           s->btn_a || s->btn_b || s->btn_x || s->btn_y ||
           s->touch_pressed || s->fire;
}
