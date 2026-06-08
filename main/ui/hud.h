#pragma once

#include <stdint.h>

void hud_init(void);
void hud_draw_string(uint16_t* fb, int x, int y, const char* str, uint16_t color);
void hud_draw_char(uint16_t* fb, int x, int y, char c, uint16_t color);
void hud_draw_rect(uint16_t* fb, int x, int y, int w, int h, uint16_t color);
void hud_draw_bar(uint16_t* fb, int x, int y, int w, int h, int value, int max_val, uint16_t fg, uint16_t bg);
void hud_draw_crosshair(uint16_t* fb, int cx, int cy, uint16_t color);
void hud_draw_reload_indicator(uint16_t* fb, int x, int y, int progress, int max_progress);
