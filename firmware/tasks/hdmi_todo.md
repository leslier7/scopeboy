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
- [x] Add `option(USE_HDMI "Use HDMI output instead of TFT" OFF)`
- [x] When `USE_HDMI=ON`: `add_subdirectory(libs/pico_hdmi)`, add `DISPLAY_HDMI` compile definition, link `pico_hdmi`
- [x] Add `display.c` to `target_sources` in both branches

### display.h (new file)
- [x] Declare `display_init()`
- [x] Declare `display_fillScreen(uint16_t color)`
- [x] Declare `display_fillRect(short x, short y, short w, short h, uint16_t color)`
- [x] Declare `display_drawPixel(short x, short y, uint16_t color)`
- [x] Declare `display_drawLine(short x0, short y0, short x1, short y1, uint16_t color)`
- [x] Declare `display_drawFastHLine(short x, short y, short w, uint16_t color)`
- [x] Declare `display_drawFastVLine(short x, short y, short h, uint16_t color)`
- [x] Declare `display_setCursor(short x, short y)`
- [x] Declare `display_setTextColor(uint16_t color)`
- [x] Declare `display_setTextSize(uint8_t size)`
- [x] Declare `display_writeString(char *str)`

### display.c (new file)

**TFT path (`#ifndef DISPLAY_HDMI`)**
- [x] `display_init()` → `tft_init_hw(); tft_begin(); tft_setRotation(3); tft_fillScreen(TFT_BLACK);`
- [x] Thin wrappers for all other `display_*` → `tft_*`

**HDMI path (`#ifdef DISPLAY_HDMI`)**
- [x] Declare `static uint16_t framebuf[240][320]`
- [x] Implement `display_drawPixel` writing to `framebuf[y][x]`
- [x] Implement `display_fillScreen` via `memset`
- [x] Implement `display_fillRect` via row-fill loop
- [x] Implement `display_drawFastHLine` via row-fill
- [x] Implement `display_drawFastVLine` via column-fill
- [x] Implement `display_drawLine` via Bresenham's algorithm
- [x] Implement text state (`cursor_x`, `cursor_y`, `text_color`, `text_size`)
- [x] Implement `display_drawChar()` using `glcdfont.c` bitmap data (adapt from `TFTMaster.c:tft_drawChar`)
- [x] Implement `display_writeString` calling `display_drawChar` per character
- [x] Implement `scanline_cb`: source row = `active_line >> 1`, pixel-double each of 320 pixels into 640-wide line_buffer (`*dst++ = (px << 16) | px`)
- [x] Implement `display_init()`: `memset` framebuf, `video_output_init(320, 240)`, `video_output_set_scanline_callback(scanline_cb)`

### Final_Project.c
- [x] Replace `#include "TFTMaster.h"` with `#include "display.h"`
- [x] Replace `tft_init_hw(); tft_begin(); tft_setRotation(3); tft_fillScreen(TFT_BLACK);` with `display_init();`
- [x] Replace all `tft_fillScreen` calls with `display_fillScreen`
- [x] Replace all `tft_fillRect` calls with `display_fillRect`
- [x] Replace all `tft_drawPixel` calls with `display_drawPixel`
- [x] Replace all `tft_drawLine` calls with `display_drawLine`
- [x] Replace all `tft_drawFastHLine` calls with `display_drawFastHLine`
- [x] Replace all `tft_drawFastVLine` calls with `display_drawFastVLine`
- [x] Replace all `tft_setCursor` calls with `display_setCursor`
- [x] Replace all `tft_setTextColor` calls with `display_setTextColor`
- [x] Replace all `tft_setTextSize` calls with `display_setTextSize`
- [x] Replace all `tft_writeString` calls with `display_writeString`
- [x] Wrap `core0_entry` / `core1_entry` with `#ifdef DISPLAY_HDMI` / `#else`:
  - HDMI: `core1_entry()` calls `video_output_core1_run()` only; `core0_entry()` adds all 4 threads (trigger, graphics, blinky, fft_calc)
  - TFT: existing split unchanged (trigger+graphics on core 0, blinky+fft on core 1)
- [x] Add `#include "pico_hdmi/video_output.h"` inside `#ifdef DISPLAY_HDMI` guard

### Verification
- [x] TFT build (default): `cmake .. && ninja -C build` compiles without errors
- [x] HDMI build: `cmake -DUSE_HDMI=ON .. && ninja -C build` compiles without errors
- [ ] HDMI: 640×480 image visible on monitor — scope grid, waveform, and menu all render correctly
- [ ] HDMI: FFT mode works (runs on Core 0 alongside graphics)

---

## HDMI Debug Log

### Root cause found and fixed

The HDMI signal was not appearing because `display_init()` and `multicore_launch_core1()` were called *after* I2C device calls in `main()`. When the Adafruit seesaw gamepad is not connected, `seesaw_pin_mode_bulk()` blocks indefinitely on `i2c_write_blocking()` waiting for an ACK that never comes — so `display_init()` is never reached.

**Fixes applied (all in `Final_Project.c`):**

1. **`display_init()` + `multicore_launch_core1()` moved to the top of `main()`**, before any I2C device calls. HDMI is now live before peripheral discovery, so a missing or slow device cannot prevent the display from starting.

2. **Seesaw I2C calls switched to `_until` variants** with a 5 ms timeout (`SEESAW_TIMEOUT_MS`). If the seesaw is not plugged in, all three functions (`seesaw_pin_mode_bulk`, `seesaw_read_buttons`, `seesaw_read_analog`) return their error values quickly instead of hanging. The oscilloscope UI remains visible; buttons simply have no effect.

3. **`scanline_cb` placed in `__scratch_x`** (`display.c`) to match the `__scratch_x` placement of the library's DMA IRQ handler and avoid XIP flash latency on the first callback invocations.

**How the debug was done:**
- Added a solid red test pattern to `display_init()` to distinguish "no signal" from "signal but black framebuffer"
- Stripped `main()` down to `display_init()` only — showed red, confirming the HDMI pipeline itself works
- Re-added peripherals one group at a time; I2C + seesaw was the failing group
- Bisected to `seesaw_pin_mode_bulk` as the hanging call (rotary is pure GPIO; DAC I2C works because the chips are on the board)
- Confirmed fix: `display_init()` first + seesaw timeout → solid red with all peripherals present

**Current state of `display.c` test pattern:** `display_init()` still fills the framebuffer with solid red (0xF800). This must be removed before shipping — see next section.

---

## Remaining tasks to get the UI working on HDMI

### display.c — Remove test pattern
- [x] In `display_init()`, remove the red fill loop and replace with `memset(framebuf, 0, sizeof(framebuf))` so the screen starts black like the TFT build

### Final_Project.c — Verify ProtoThreads graphics render
- [ ] Flash with the test pattern removed; confirm `protothread_graphics` draws the scope grid and waveform to the framebuffer. If the screen stays black, add a `printf` at the start of `protothread_graphics` and check serial output (115200 baud via USB CDC or UART) to confirm the thread is running.
- [ ] Confirm `protothread_trigger` fires and `trigger_semaphore` is signalled — graphics thread waits on it before the first draw
- [ ] Confirm FFT mode works (all 4 ProtoThreads on Core 0 in HDMI mode)

### Input — Seesaw graceful degradation
- [ ] The seesaw timeout fix means the UI is visible without the gamepad plugged in. Verify that plugging the seesaw back in while running restores button input (it should, since each `seesaw_read_buttons` call retries with a fresh timeout)
- [x] Added `seesaw_present` flag set during `seesaw_pin_mode_bulk` — skips all 3 read calls in `handleInput()` when init failed, eliminating the 15 ms per-frame timeout overhead in HDMI mode with no gamepad

### Verification
- [ ] TFT build still compiles and works: `cmake .. -B build && ninja -C build`
- [ ] HDMI build: `cmake -DUSE_HDMI=ON .. -B build && ninja -C build`, flash, confirm scope grid + waveform visible on monitor
- [ ] HDMI build with seesaw unplugged: display still works, no hang
