#include "menus.h"
#include "hud.h"
#include "../config.h"
#include <cstdio>
#include <cstring>

static MenuType s_current = MENU_TITLE;
static int s_selected = 0;
static int s_repeat_delay = 0;

void menus_init(void) {
    s_current = MENU_TITLE;
    s_selected = 0;
    s_repeat_delay = 0;
}

MenuType menus_get_current(void) { return s_current; }
void menus_set_current(MenuType type) {
    s_current = type;
    s_selected = 0;
    s_repeat_delay = 0;
}
int menus_get_selection(void) { return s_selected; }
void menus_set_selection(int sel) { s_selected = sel; }

void menus_update(const InputState* input) {
    if (s_repeat_delay > 0) s_repeat_delay--;

    bool up = input->dpad_up || (input->stick_y < -0.5f);
    bool down = input->dpad_down || (input->stick_y > 0.5f);

    if (s_current == MENU_TITLE) {
        if (up && s_repeat_delay == 0) {
            s_selected = (s_selected - 1 + TITLE_COUNT) % TITLE_COUNT;
            s_repeat_delay = 12;
        }
        if (down && s_repeat_delay == 0) {
            s_selected = (s_selected + 1) % TITLE_COUNT;
            s_repeat_delay = 12;
        }
    } else if (s_current == MENU_PAUSE) {
        if (up && s_repeat_delay == 0) {
            s_selected = (s_selected - 1 + PAUSE_COUNT) % PAUSE_COUNT;
            s_repeat_delay = 12;
        }
        if (down && s_repeat_delay == 0) {
            s_selected = (s_selected + 1) % PAUSE_COUNT;
            s_repeat_delay = 12;
        }
    } else if (s_current == MENU_SAVE_SELECT) {
        if (up && s_repeat_delay == 0) {
            s_selected = (s_selected - 1 + SAVE_MAX_SLOTS) % SAVE_MAX_SLOTS;
            s_repeat_delay = 12;
        }
        if (down && s_repeat_delay == 0) {
            s_selected = (s_selected + 1) % SAVE_MAX_SLOTS;
            s_repeat_delay = 12;
        }
    }
}

static void draw_menu_box(uint16_t* fb, int x, int y, int w, int h) {
    hud_draw_rect(fb, x, y, w, h, 0x0000);
    hud_draw_rect(fb, x, y, w, 1, 0x4208);
    hud_draw_rect(fb, x, y + h - 1, w, 1, 0x4208);
    hud_draw_rect(fb, x, y, 1, h, 0x4208);
    hud_draw_rect(fb, x + w - 1, y, 1, h, 0x4208);
}

void menus_draw_title(uint16_t* fb, int selected) {
    for (int y = 40; y < 200; y++) {
        for (int x = 20; x < 220; x++) {
            fb[y * DISPLAY_WIDTH + x] = (fb[y * DISPLAY_WIDTH + x] >> 2) & 0x1CE7;
        }
    }

    draw_menu_box(fb, 30, 45, 180, 150);

    hud_draw_string(fb, 48, 52, "ESCAPE FROM", 0xFFE0);
    hud_draw_string(fb, 60, 65, "BUGWORLD", 0xF800);

    const char* items[] = {"NEW GAME", "CONTINUE", "OPTIONS", "HIGH SCORES"};
    for (int i = 0; i < TITLE_COUNT; i++) {
        int y_pos = 90 + i * 18;
        if (i == selected) {
            hud_draw_rect(fb, 40, y_pos - 1, 160, 14, 0x4208);
            hud_draw_string(fb, 50, y_pos, items[i], 0xFFE0);
        } else {
            hud_draw_string(fb, 50, y_pos, items[i], 0xFFFF);
        }
    }

    hud_draw_string(fb, 40, 170, "A:SELECT", 0x8410);
}

void menus_draw_pause(uint16_t* fb, int selected) {
    for (int y = 45; y < 200; y++) {
        for (int x = 30; x < 210; x++) {
            fb[y * DISPLAY_WIDTH + x] = (fb[y * DISPLAY_WIDTH + x] >> 1) & 0x3DEF;
        }
    }

    draw_menu_box(fb, 35, 50, 170, 140);

    hud_draw_string(fb, 82, 55, "PAUSED", 0xFFE0);

    const char* items[] = {"RESUME", "SAVE", "OPTIONS", "QUIT"};
    for (int i = 0; i < PAUSE_COUNT; i++) {
        int y_pos = 75 + i * 20;
        if (i == selected) {
            hud_draw_rect(fb, 45, y_pos - 1, 150, 16, 0x4208);
            hud_draw_string(fb, 55, y_pos, items[i], 0xFFE0);
        } else {
            hud_draw_string(fb, 55, y_pos, items[i], 0xFFFF);
        }
    }

    hud_draw_string(fb, 45, 170, "A:SEL B:BACK", 0x8410);
}

void menus_draw_game_over(uint16_t* fb, int score) {
    for (int y = 50; y < 190; y++) {
        for (int x = 25; x < 215; x++) {
            fb[y * DISPLAY_WIDTH + x] = (fb[y * DISPLAY_WIDTH + x] >> 2) & 0x1CE7;
        }
    }

    draw_menu_box(fb, 30, 55, 180, 130);

    hud_draw_string(fb, 64, 65, "GAME OVER", 0xF800);

    hud_draw_string(fb, 60, 95, "SCORE:", 0xFFFF);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", score);
    hud_draw_string(fb, 60, 110, buf, 0xFFE0);

    hud_draw_string(fb, 40, 150, "TAP TO CONTINUE", 0x8410);
}

void menus_draw_win(uint16_t* fb, int score) {
    for (int y = 40; y < 200; y++) {
        for (int x = 20; x < 220; x++) {
            fb[y * DISPLAY_WIDTH + x] = (fb[y * DISPLAY_WIDTH + x] >> 2) & 0x1CE7;
        }
    }

    draw_menu_box(fb, 25, 45, 190, 150);

    hud_draw_string(fb, 52, 55, "VICTORY!", 0x07E0);
    hud_draw_string(fb, 40, 75, "THE BUGWORLD", 0x07FF);
    hud_draw_string(fb, 40, 88, "IS SAVED!", 0x07FF);

    hud_draw_string(fb, 50, 110, "FINAL SCORE:", 0xFFFF);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", score);
    hud_draw_string(fb, 50, 125, buf, 0xFFE0);

    hud_draw_string(fb, 40, 160, "TAP TO CONTINUE", 0x8410);
}

void menus_draw_options(uint16_t* fb, int selected, uint8_t volume, uint8_t brightness, bool sound_on, int difficulty) {
    for (int y = 30; y < 210; y++) {
        for (int x = 15; x < 225; x++) {
            fb[y * DISPLAY_WIDTH + x] = (fb[y * DISPLAY_WIDTH + x] >> 2) & 0x1CE7;
        }
    }

    draw_menu_box(fb, 20, 35, 200, 170);

    hud_draw_string(fb, 72, 40, "OPTIONS", 0xFFE0);

    const char* labels[] = {"VOLUME", "BRIGHT", "SOUND", "DIFFICULTY"};
    for (int i = 0; i < 4; i++) {
        int y_pos = 60 + i * 22;
        if (i == selected) {
            hud_draw_rect(fb, 30, y_pos - 1, 180, 18, 0x4208);
            hud_draw_string(fb, 35, y_pos, labels[i], 0xFFE0);
        } else {
            hud_draw_string(fb, 35, y_pos, labels[i], 0xFFFF);
        }

        char val_buf[16];
        switch (i) {
            case 0: snprintf(val_buf, sizeof(val_buf), "%d", volume); break;
            case 1: snprintf(val_buf, sizeof(val_buf), "%d", brightness); break;
            case 2: snprintf(val_buf, sizeof(val_buf), "%s", sound_on ? "ON" : "OFF"); break;
            case 3: {
                const char* diff_names[] = {"EASY", "NORMAL", "HARD"};
                snprintf(val_buf, sizeof(val_buf), "%s", diff_names[difficulty % 3]);
                break;
            }
        }
        hud_draw_string(fb, 150, y_pos, val_buf, 0x07FF);
    }

    hud_draw_string(fb, 30, 185, "A:CHANGE B:BACK", 0x8410);
}

void menus_draw_high_scores(uint16_t* fb, const int* scores) {
    for (int y = 40; y < 200; y++) {
        for (int x = 20; x < 220; x++) {
            fb[y * DISPLAY_WIDTH + x] = (fb[y * DISPLAY_WIDTH + x] >> 2) & 0x1CE7;
        }
    }

    draw_menu_box(fb, 30, 45, 180, 150);

    hud_draw_string(fb, 52, 52, "HIGH SCORES", 0xFFE0);

    for (int i = 0; i < 3; i++) {
        int y_pos = 80 + i * 25;
        char rank_buf[8];
        snprintf(rank_buf, sizeof(rank_buf), "%d.", i + 1);
        hud_draw_string(fb, 45, y_pos, rank_buf, 0xFFFF);

        char score_buf[16];
        snprintf(score_buf, sizeof(score_buf), "%d", scores[i]);
        hud_draw_string(fb, 70, y_pos, score_buf, 0xFFE0);
    }

    hud_draw_string(fb, 50, 165, "B:BACK", 0x8410);
}

void menus_draw_save_select(uint16_t* fb, int selected, const bool* slot_exists) {
    for (int y = 50; y < 190; y++) {
        for (int x = 30; x < 210; x++) {
            fb[y * DISPLAY_WIDTH + x] = (fb[y * DISPLAY_WIDTH + x] >> 1) & 0x3DEF;
        }
    }

    draw_menu_box(fb, 35, 55, 170, 130);

    hud_draw_string(fb, 62, 60, "SAVE SLOT", 0xFFE0);

    for (int i = 0; i < SAVE_MAX_SLOTS; i++) {
        int y_pos = 85 + i * 22;
        char label[16];
        snprintf(label, sizeof(label), "SLOT %d %s", i + 1, slot_exists[i] ? "*" : " ");

        if (i == selected) {
            hud_draw_rect(fb, 45, y_pos - 1, 150, 18, 0x4208);
            hud_draw_string(fb, 55, y_pos, label, 0xFFE0);
        } else {
            hud_draw_string(fb, 55, y_pos, label, 0xFFFF);
        }
    }

    hud_draw_string(fb, 45, 165, "A:SAVE B:BACK", 0x8410);
}
