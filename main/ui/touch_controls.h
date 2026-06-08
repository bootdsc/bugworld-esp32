#pragma once

#include <stdint.h>

void touch_controls_init(void);
void touch_controls_draw(uint16_t* fb, int touch_x, int touch_y, bool touching,
                         float stick_x, float stick_y, bool fire_pressed);
