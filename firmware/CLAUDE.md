# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Scopeboy is an oscilloscope built on the **Raspberry Pi Pico 2** (RP2350). It captures analog signals via an external 8-bit ADC wired to GPIO32–39, displays waveforms on an ILI9340 TFT over SPI (driven by PIO), and accepts user input from a rotary encoder (GPIO23–25) and an Adafruit Mini Gamepad over I2C Seesaw (address 0x50, i2c1, SDA=GPIO20/SCL=GPIO21).

## Build & Flash

The build system uses the Pico SDK 2.2.0 with CMake + Ninja. The `build/` directory is pre-configured.

**Build:**
```bash
~/.pico-sdk/ninja/v1.12.1/ninja -C build
```

**Flash via picotool** (Pico must be in BOOTSEL mode):
```bash
~/.pico-sdk/picotool/2.2.0-a4/picotool/picotool load build/Final_Project.uf2 -fx
```

**Flash via OpenOCD** (SWD debugger attached):
```bash
~/.pico-sdk/openocd/0.12.0+dev/openocd \
  -s ~/.pico-sdk/openocd/0.12.0+dev/scripts \
  -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
  -c "adapter speed 5000; program \"build/Final_Project.elf\" verify reset exit"
```

**Reconfigure CMake** (only needed after adding/removing source files):
```bash
cd build && cmake ..
```

Serial output is available on both UART and USB CDC.

## Architecture

### Source modules

| File | Responsibility |
|---|---|
| `Final_Project.c` | Main entry point. Owns the ProtoThreads scheduler, menu state machine, waveform rendering, FFT display, and the Snake easter-egg game. |
| `adc.c / adc.h` | Configures a dual-DMA ring that samples GPIO32–39 at 500 kHz into `capture_buf[320]`. Gain is set by toggling analog mux select pins SEL_0 (GPIO5) / SEL_1 (GPIO6). (Will be updated to use parallel ADC in the future) |
| `trigger.c / trigger.h` | Rising-edge GPIO ISR on GPIO4. On fire, calls `trigger_isr()` (sets `trigger_fired`, disarms) then signals `trigger_semaphore`. `trigger_copy()` snapshots `capture_buf` → `frame_buf`. |
| `dac.c / dac.h` | Two DAC5571 8-bit I2C DACs on I2C1 (shared with the seesaw bus, SDA=GPIO20/SCL=GPIO21). `CHAN_TRIG` (0x4D) sets trigger threshold; `CHAN_OFFSET` (0x4C) sets signal offset. |
| `TFTMaster.c / TFTMaster.h` | ILI9340 driver. Uses the PIO SPI state machine defined in `SPIPIO.pio`. |
| `SPIPIO.pio` | PIO SPI CPHA=0 program that drives the TFT SCK/MOSI lines. |
| `glcdfont.c` | Bitmap font data for TFT text rendering. |
| `pt_cornell_rp2040_v1_4.h` | Cornell ECE ProtoThreads library for cooperative multithreading on the RP2040/RP2350. |
| `libs / pico_hdmi / include / video_output.c` | Pico HDMI library header file |

### Concurrency model

The firmware uses **ProtoThreads** (cooperative, no preemption) on core 0 for the UI and display loop. The trigger ISR fires on core 0's GPIO interrupt. `trigger_semaphore` is a ProtoThreads semaphore used to synchronise the ISR with the display thread — the ISR signals it, the waveform thread waits on it before consuming `frame_buf`.

The dual-DMA chain runs autonomously: the control channel resets the data channel's write pointer; the data channel reads `GPIO_HI_IN[7:0]` at 500 kHz (paced by DMA timer 0) and wraps around `capture_buf` continuously.

### Key constants

- `CAPTURE_DEPTH` = 320 (one pixel column per sample)
- `SAMPLE_RATE_HZ` = 500 000 Hz
- Display: 320 × 240 pixels, landscape
- `PIXELS_PER_DIV` = 48 px
- ADC bus: `D0`=GPIO32 … `D7`=GPIO39 (upper GPIO bank, read via `sio_hw->gpio_hi_in`)
