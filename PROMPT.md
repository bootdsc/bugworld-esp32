# BUGWORLD - DEBUG PROMPT FOR AI MODELS

## Project
**Escape From BugWorld** - ESP-IDF game for Waveshare ESP32-S3 Touch LCD 2.8"

## GitHub Repo
https://github.com/bootdsc/bugworld-esp32

## Hardware
- ESP32-S3 (240MHz, 8MB PSRAM)
- Display: ST7789 240x320, raw SPI, MADCTL=0x40 (MX=1, X axis mirrored)
- Touch: CST328 I2C @ 0x1A, direct I2C (not esp_lcd_touch)
- I2C: SDA=1, SCL=3, 100kHz
- Pins: Display MOSI=45, SCLK=40, CS=42, DC=41, RST=39, BL=5

## CRITICAL BUGS TO FIX

### 1. Touch Input Completely Broken
- Fire button stuck ON at boot (turret auto-fires)
- Joystick values jump all over
- Touch coordinates are garbage
- Phantom presses when no finger touching

**Files to debug:**
- `main/drivers/input.cpp` - CST328 I2C driver
- `main/drivers/input.h` - InputState struct
- `main/config.h` - Touch zone definitions
- `main/ui/touch_controls.cpp` - Visual overlay

### 2. Coordinate System (CRITICAL)
ST7789 MADCTL=0x40 mirrors X axis:
- Framebuffer x=0 = PHYSICAL RIGHT
- Framebuffer x=239 = PHYSICAL LEFT
- Y is NOT mirrored (y=0=top, y=319=bottom)

CST328 reports PHYSICAL coordinates:
- touch_x=0 = physical left, touch_x=239 = physical right
- touch_y=0 = physical top, touch_y=319 = physical bottom

**Detection uses raw physical coords** (NOT framebuffer):
- Joystick zone: raw_x < 120, raw_y >= 220
- Fire zone: raw_x > 180, raw_y >= 220
- Pause zone: raw_y < 20

**Rendering uses framebuffer coords** (mirrored X):
- Joystick at fb x=179 (physical left)
- Fire button at fb x=40 (physical right)

### 3. Joystick Movement Direction
Because of X mirror:
- stick_x > 0 (drag right physically) should move dot LEFT on framebuffer
- Use `JOY_CENTER_X - jdx` not `JOY_CENTER_X + jdx`

### 4. Fire Button Edge Detection
Fire must trigger ONCE per press, not while holding:
- Use `fire_edge = fire_now && !fire_pressed_prev`
- Set `fire_pressed_prev = fire_now` each frame
- Apply `fire_edge` to shooting, not `fire_now`

### 5. Game Loop Timing
Remove FPS counter (inaccurate), add frame limiter:
```cpp
int64_t frame_start = esp_timer_get_time();
// ... game logic ...
int64_t elapsed = esp_timer_get_time() - frame_start;
if (elapsed < 16667) vTaskDelay(pdMS_TO_TICKS((16667-elapsed)/1000));
```

## WHAT HAS BEEN TRIED
1. Multiple CST328 register parsing attempts (still wrong)
2. Various zone definitions (still wrong)
3. Added touch reset sequence
4. Changed to direct I2C (transmit+receive with STOP)
5. Lowered I2C speed to 100kHz
6. Added edge detection for menus

## REQUEST
Fix the CST328 touch driver so:
1. No phantom presses when finger is off screen
2. Joystick smoothly tracks finger in bottom-left
3. Fire button fires once per press (1s cooldown)
4. Pause works when touching top 20px
5. No rapid-fire loops or stuck buttons

The project is in `main/drivers/input.cpp` and `main/config.h`.
