#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../drivers/input.h"

typedef enum {
    MENU_NONE,
    MENU_TITLE,
    MENU_PAUSE,
    MENU_GAME_OVER,
    MENU_WIN,
    MENU_OPTIONS,
    MENU_HIGH_SCORES,
    MENU_SAVE_SELECT,
} MenuType;

typedef enum {
    TITLE_NEW_GAME,
    TITLE_CONTINUE,
    TITLE_OPTIONS,
    TITLE_HIGH_SCORES,
    TITLE_COUNT
} TitleItem;

typedef enum {
    PAUSE_RESUME,
    PAUSE_SAVE,
    PAUSE_OPTIONS,
    PAUSE_QUIT,
    PAUSE_COUNT
} PauseItem;

void menus_init(void);
MenuType menus_get_current(void);
void menus_set_current(MenuType type);
int menus_get_selection(void);
void menus_set_selection(int sel);
void menus_update(const InputState* input);
void menus_draw_title(uint16_t* fb, int selected);
void menus_draw_pause(uint16_t* fb, int selected);
void menus_draw_game_over(uint16_t* fb, int score);
void menus_draw_win(uint16_t* fb, int score);
void menus_draw_options(uint16_t* fb, int selected, uint8_t volume, uint8_t brightness, bool sound_on, int difficulty);
void menus_draw_high_scores(uint16_t* fb, const int* scores);
void menus_draw_save_select(uint16_t* fb, int selected, const bool* slot_exists);
