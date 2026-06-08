#include "touch_controls.h"
#include "../config.h"
#include <cmath>
#include <cstring>

static int s_joy_visual_x = 0;
static int s_joy_visual_y = 0;
static bool s_joy_active = false;

void touch_controls_init(void) {
    s_joy_visual_x = 0;
    s_joy_visual_y = 0;
    s_joy_active = false;
}

static void draw_circle_outline(uint16_t* fb, int cx, int cy, int r, uint16_t color) {
    int x = r;
    int y = 0;
    int err = 1 - r;

    while (x >= y) {
        int points[][2] = {
            {cx + x, cy + y}, {cx - x, cy + y},
            {cx + x, cy - y}, {cx - x, cy - y},
            {cx + y, cy + x}, {cx - y, cy + x},
            {cx + y, cy - x}, {cx - y, cy - x},
        };
        for (int i = 0; i < 8; i++) {
            int px = points[i][0];
            int py = points[i][1];
            if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                fb[py * DISPLAY_WIDTH + px] = color;
            }
        }
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

static void draw_filled_circle(uint16_t* fb, int cx, int cy, int r, uint16_t color) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                int px = cx + dx;
                int py = cy + dy;
                if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                    fb[py * DISPLAY_WIDTH + px] = color;
                }
            }
        }
    }
}

static void draw_rect_area(uint16_t* fb, int x, int y, int w, int h, uint16_t color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < DISPLAY_WIDTH && py >= DISPLAY_HEIGHT - CONTROL_AREA_H && py < DISPLAY_HEIGHT) {
                fb[py * DISPLAY_WIDTH + px] = color;
            }
        }
    }
}

/* NOTE: touch_x and touch_y are RAW coordinates (physical left=0, right=239).
 * Joystick is raw x < 100, fire is raw x > 140.
 * Rendering uses framebuffer coordinates (MADCTL=0x40 mirrors X).
 */
void touch_controls_draw(uint16_t* fb, int touch_x, int touch_y, bool touching,
                         float stick_x, float stick_y, bool fire_pressed) {
    draw_rect_area(fb, 0, DISPLAY_HEIGHT - CONTROL_AREA_H, DISPLAY_WIDTH, CONTROL_AREA_H, 0x1082);

    draw_rect_area(fb, 0, DISPLAY_HEIGHT - CONTROL_AREA_H, DISPLAY_WIDTH, 1, 0x4208);

    /* Joystick: physical left.
     * Framebuffer is mirrored (MX=1): fb x increases = physical left.
     * So stick_x > 0 (drag right) must DECREASE fb x to move dot right on screen. */
    int jdx = (int)(stick_x * JOY_RADIUS_X * 0.5f);
    int jdy = (int)(stick_y * JOY_RADIUS_Y * 0.5f);
    int dot_x = JOY_CENTER_X - jdx;  /* MINUS because of X mirror */
    int dot_y = JOY_CENTER_Y + jdy;

    if (touching && touch_x < 120 && touch_y >= 220) {
        draw_filled_circle(fb, dot_x, dot_y, 12, 0xFFE0);
        draw_circle_outline(fb, dot_x, dot_y, 12, 0xFFFF);
    } else {
        draw_filled_circle(fb, JOY_CENTER_X, JOY_CENTER_Y, 10, 0x8410);
    }

    /* Fire button: physical right, fully visible */
    int fire_r = FIRE_RADIUS;
    if (fire_pressed) {
        draw_filled_circle(fb, FIRE_CENTER_X, FIRE_CENTER_Y, fire_r, 0xF800);
        draw_circle_outline(fb, FIRE_CENTER_X, FIRE_CENTER_Y, fire_r + 2, 0xFD20);
    } else {
        draw_circle_outline(fb, FIRE_CENTER_X, FIRE_CENTER_Y, fire_r, 0xF800);
        draw_circle_outline(fb, FIRE_CENTER_X, FIRE_CENTER_Y, fire_r - 2, 0xA000);
    }

    int text_y = DISPLAY_HEIGHT - 14;
    uint16_t label_color = 0x6B4D;

    const char* joy_label = "AIM";
    int joy_label_x = JOY_CENTER_X - 9;
    for (const char* c = joy_label; *c; c++) {
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                int px = joy_label_x + col;
                int py = text_y + row;
                if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                    fb[py * DISPLAY_WIDTH + px] = label_color;
                }
            }
        }
        joy_label_x += 4;
    }

    const char* fire_label = "FIRE";
    int fire_label_x = FIRE_CENTER_X - 12;
    for (const char* c = fire_label; *c; c++) {
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                int px = fire_label_x + col;
                int py = text_y + row;
                if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                    fb[py * DISPLAY_WIDTH + px] = label_color;
                }
            }
        }
        fire_label_x += 4;
    }
}
