#pragma once

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320

#define GAME_AREA_W     240
#define GAME_AREA_H     240
#define CONTROL_AREA_H  100
#define PAUSE_ZONE_H    20

#define DISPLAY_MOSI    45
#define DISPLAY_SCLK    40
#define DISPLAY_CS      42
#define DISPLAY_DC      41
#define DISPLAY_RST     39
#define DISPLAY_BL      5
#define DISPLAY_SPI_HZ  (40 * 1000 * 1000)

#define TOUCH_SDA       1
#define TOUCH_SCL       3
#define TOUCH_INT       4
#define TOUCH_RST       2
#define TOUCH_I2C_HZ    (100 * 1000)

#define AUDIO_BCK       48
#define AUDIO_LRCK      38
#define AUDIO_DIN       47
#define AUDIO_SAMPLE_RATE 22050

#define BTN_UP          18
#define BTN_DOWN        15
#define BTN_LEFT        43
#define BTN_RIGHT       44
#define BTN_A           21
#define BTN_B           14
#define BTN_X           16
#define BTN_Y           17

/* =============================================================================
 * TOUCH CONTROL POSITIONS — IMPORTANT: Framebuffer vs Physical Screen
 * =============================================================================
 * The ST7789 display uses MADCTL=0x40 (MX=1), which mirrors the X axis.
 * This means:
 *   - Framebuffer x=0  appears on the PHYSICAL RIGHT side of the screen
 *   - Framebuffer x=239 appears on the PHYSICAL LEFT side of the screen
 *   - Y is NOT mirrored (y=0 is physical top, y=319 is physical bottom)
 *
 * The CST328 touch chip reports coordinates in PHYSICAL space:
 *   - touch_x=0 is physical left, touch_x=239 is physical right
 *   - touch_y=0 is physical top,    touch_y=319 is physical bottom
 *
 * input_scan() uses raw physical coords for zone detection.
 * touch_controls_draw() uses framebuffer coords (mirrored X).
 * ============================================================================= */
/* Joystick: centered in 120x120 box at far bottom left (physical).
 * Physical left edge x=0. 120px box = x=0..119. Center = x=60.
 * Framebuffer mirrored: physical x=60 -> fb x=239-60=179. */
#define JOY_CENTER_X    179
#define JOY_CENTER_Y    270
#define JOY_RADIUS_X    60
#define JOY_RADIUS_Y    60
#define JOY_DEADZONE    0.15f

/* Fire button: 20px from physical right edge, fully visible.
 * Physical right edge x=239. Button radius 20. Center at x=239-20-20=199.
 * Framebuffer mirrored: physical x=199 -> fb x=239-199=40. */
#define FIRE_CENTER_X   40
#define FIRE_CENTER_Y   270
#define FIRE_RADIUS     24

#define MAX_ENEMIES     15
#define MAX_PROJECTILES 30
#define MAX_EFFECTS     24
#define MAX_GENERATORS  5

#define TURRET_PAN_MIN  (-60.0f)
#define TURRET_PAN_MAX  (60.0f)
#define TURRET_TILT_MIN (-10.0f)
#define TURRET_TILT_MAX (30.0f)
#define TURRET_SPEED    2.5f

#define SAVE_MAGIC      0x42574753
#define SAVE_VERSION    1
#define SAVE_MAX_SLOTS  3
