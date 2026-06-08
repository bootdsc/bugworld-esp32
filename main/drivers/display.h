#pragma once

#include <stdint.h>
#include <stdbool.h>

bool display_init(void);
void display_set_brightness(uint8_t level);
uint16_t* display_get_framebuffer(void);
void display_flush_full(uint16_t* portrait_buf);
