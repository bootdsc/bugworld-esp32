#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    /* Physical buttons (active low, pull-up) */
    bool dpad_up, dpad_down, dpad_left, dpad_right;
    bool btn_a, btn_b, btn_x, btn_y;

    /* Touch state */
    bool touch_active;
    int touch_x;      /* Raw physical coords (0..239, 0..319) */
    int touch_y;
    bool touch_pressed;

    /* Derived inputs */
    float stick_x;    /* -1..1 */
    float stick_y;    /* -1..1 */
    bool fire;        /* true = fire button pressed */
    bool pause;       /* true = pause requested */
} InputState;

bool input_init(void);
void input_scan(InputState* out);
bool input_any_pressed(const InputState* s);
