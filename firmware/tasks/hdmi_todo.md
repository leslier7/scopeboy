# HDMI Output Migration

Switch from TFT-only output to a compile-time selectable TFT / HDMI display using the `pico_hdmi` library. Build with `cmake -DUSE_HDMI=ON ..` for HDMI, default remains TFT.

**Notes:**
- TFT (GPIO 0/1/2/27) and HDMI (GPIO 12-19) are on separate pins — no conflict.
- `pico_hdmi` library HSTX lane assignments already match the PCB (D2→12-13, CLK→14-15, D1→16-17, D0→18-19). No library edits needed.
- TFTMaster.h pins are already updated for the new PCB.
- HDMI mode uses a 320×240 RGB565 framebuffer (~150 KB) pixel-doubled to 640×480 in the scanline callback.
- HDMI takes over Core 1, so `protothread_blinky` and `protothread_fft_calc` move to Core 0 in HDMI mode.

---

## Tasks

### CMakeLists.txt
- [ ] Add `option(USE_HDMI "Use HDMI output instead of TFT" OFF)`
- [ ] When `USE_HDMI=ON`: `add_subdirectory(libs/pico_hdmi)`, add `DISPLAY_HDMI` compile definition, link `pico_hdmi`
- [ ] Add `display.c` to `target_sources` in both branches

### display.h (new file)
- [ ] Declare `display_init()`
- [ ] Declare `display_fillScreen(uint16_t color)`
- [ ] Declare `display_fillRect(short x, short y, short w, short h, uint16_t color)`
- [ ] Declare `display_drawPixel(short x, short y, uint16_t color)`
- [ ] Declare `display_drawLine(short x0, short y0, short x1, short y1, uint16_t color)`
- [ ] Declare `display_drawFastHLine(short x, short y, short w, uint16_t color)`
- [ ] Declare `display_drawFastVLine(short x, short y, short h, uint16_t color)`
- [ ] Declare `display_setCursor(short x, short y)`
- [ ] Declare `display_setTextColor(uint16_t color)`
- [ ] Declare `display_setTextSize(uint8_t size)`
- [ ] Declare `display_writeString(char *str)`

### display.c (new file)

**TFT path (`#ifndef DISPLAY_HDMI`)**
- [ ] `display_init()` → `tft_init_hw(); tft_begin(); tft_setRotation(3); tft_fillScreen(TFT_BLACK);`
- [ ] Thin wrappers for all other `display_*` → `tft_*`

**HDMI path (`#ifdef DISPLAY_HDMI`)**
- [ ] Declare `static uint16_t framebuf[240][320]`
- [ ] Implement `display_drawPixel` writing to `framebuf[y][x]`
- [ ] Implement `display_fillScreen` via `memset`
- [ ] Implement `display_fillRect` via row-fill loop
- [ ] Implement `display_drawFastHLine` via row-fill
- [ ] Implement `display_drawFastVLine` via column-fill
- [ ] Implement `display_drawLine` via Bresenham's algorithm
- [ ] Implement text state (`cursor_x`, `cursor_y`, `text_color`, `text_size`)
- [ ] Implement `display_drawChar()` using `glcdfont.c` bitmap data (adapt from `TFTMaster.c:tft_drawChar`)
- [ ] Implement `display_writeString` calling `display_drawChar` per character
- [ ] Implement `scanline_cb`: source row = `active_line >> 1`, pixel-double each of 320 pixels into 640-wide line_buffer (`*dst++ = (px << 16) | px`)
- [ ] Implement `display_init()`: `memset` framebuf, `video_output_init(640, 480)`, `video_output_set_scanline_callback(scanline_cb)`

### Final_Project.c
- [ ] Replace `#include "TFTMaster.h"` with `#include "display.h"`
- [ ] Replace `tft_init_hw(); tft_begin(); tft_setRotation(3); tft_fillScreen(TFT_BLACK);` with `display_init();`
- [ ] Replace all `tft_fillScreen` calls with `display_fillScreen`
- [ ] Replace all `tft_fillRect` calls with `display_fillRect`
- [ ] Replace all `tft_drawPixel` calls with `display_drawPixel`
- [ ] Replace all `tft_drawLine` calls with `display_drawLine`
- [ ] Replace all `tft_drawFastHLine` calls with `display_drawFastHLine`
- [ ] Replace all `tft_drawFastVLine` calls with `display_drawFastVLine`
- [ ] Replace all `tft_setCursor` calls with `display_setCursor`
- [ ] Replace all `tft_setTextColor` calls with `display_setTextColor`
- [ ] Replace all `tft_setTextSize` calls with `display_setTextSize`
- [ ] Replace all `tft_writeString` calls with `display_writeString`
- [ ] Wrap `core0_entry` / `core1_entry` with `#ifdef DISPLAY_HDMI` / `#else`:
  - HDMI: `core1_entry()` calls `video_output_core1_run()` only; `core0_entry()` adds all 4 threads (trigger, graphics, blinky, fft_calc)
  - TFT: existing split unchanged (trigger+graphics on core 0, blinky+fft on core 1)
- [ ] Add `#include "pico_hdmi/video_output.h"` inside `#ifdef DISPLAY_HDMI` guard

### Verification
- [ ] TFT build (default): `cmake .. && ninja -C build` compiles without errors
- [ ] HDMI build: `cmake -DUSE_HDMI=ON .. && ninja -C build` compiles without errors
- [ ] HDMI: 640×480 image visible on monitor — scope grid, waveform, and menu all render correctly
- [ ] HDMI: FFT mode works (runs on Core 0 alongside graphics)
