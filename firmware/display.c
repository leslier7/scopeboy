#include "display.h"

#ifndef DISPLAY_HDMI

// ============================================================
// TFT path — thin wrappers over TFTMaster
// ============================================================

#include "TFTMaster.h"

void display_init(void) {
    tft_init_hw();
    tft_begin();
    tft_setRotation(3);
    tft_fillScreen(ILI9340_BLACK);
}

void display_fillScreen(uint16_t color)                                           { tft_fillScreen(color); }
void display_fillRect(short x, short y, short w, short h, uint16_t color)        { tft_fillRect(x, y, w, h, color); }
void display_drawPixel(short x, short y, uint16_t color)                          { tft_drawPixel(x, y, color); }
void display_drawLine(short x0, short y0, short x1, short y1, uint16_t color)    { tft_drawLine(x0, y0, x1, y1, color); }
void display_drawFastHLine(short x, short y, short w, uint16_t color)             { tft_drawFastHLine(x, y, w, color); }
void display_drawFastVLine(short x, short y, short h, uint16_t color)             { tft_drawFastVLine(x, y, h, color); }
void display_setCursor(short x, short y)                                          { tft_setCursor(x, y); }
void display_setTextColor(uint16_t color)                                         { tft_setTextColor(color); }
void display_setTextSize(uint8_t size)                                            { tft_setTextSize(size); }
void display_writeString(char *str)                                               { tft_writeString(str); }

#else

// ============================================================
// HDMI path — 320×240 RGB565 framebuffer, pixel-doubled to 640×480
// ============================================================

#include "pico_hdmi/video_output.h"
#include "pico_hdmi/hstx_data_island_queue.h"
#include "pico/platform/sections.h"
#include <string.h>
#include <stdlib.h>
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#include "glcdfont.c"

static uint16_t framebuf[240][320];

static short cursor_x = 0;
static short cursor_y = 0;
static uint16_t text_color = 0xFFFF;
static uint8_t text_size = 1;

void display_drawPixel(short x, short y, uint16_t color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 240)
        framebuf[y][x] = color;
}

void display_fillScreen(uint16_t color) {
    for (int y = 0; y < 240; y++)
        for (int x = 0; x < 320; x++)
            framebuf[y][x] = color;
}

void display_fillRect(short x, short y, short w, short h, uint16_t color) {
    for (short row = y; row < y + h; row++)
        for (short col = x; col < x + w; col++)
            display_drawPixel(col, row, color);
}

void display_drawFastHLine(short x, short y, short w, uint16_t color) {
    for (short col = x; col < x + w; col++)
        display_drawPixel(col, y, color);
}

void display_drawFastVLine(short x, short y, short h, uint16_t color) {
    for (short row = y; row < y + h; row++)
        display_drawPixel(x, row, color);
}

void display_drawLine(short x0, short y0, short x1, short y1, uint16_t color) {
    short dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    short dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    short err = dx + dy;
    while (1) {
        display_drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        short e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void display_setCursor(short x, short y)    { cursor_x = x; cursor_y = y; }
void display_setTextColor(uint16_t color)   { text_color = color; }
void display_setTextSize(uint8_t size)      { text_size = size > 0 ? size : 1; }

static void display_drawChar(short x, short y, unsigned char c, uint16_t color, uint8_t size) {
    for (int i = 0; i < 6; i++) {
        unsigned char line = (i == 5) ? 0x00 : pgm_read_byte(font + (c * 5) + i);
        for (int j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1)
                    display_drawPixel(x + i, y + j, color);
                else
                    display_fillRect(x + i * size, y + j * size, size, size, color);
            }
            line >>= 1;
        }
    }
}

void display_writeString(char *str) {
    while (*str) {
        char c = *str++;
        if (c == '\n') {
            cursor_y += text_size * 8;
            cursor_x = 0;
        } else if (c != '\r') {
            display_drawChar(cursor_x, cursor_y, c, text_color, text_size);
            cursor_x += text_size * 6;
        }
    }
}

static void __scratch_x("") scanline_cb(uint32_t v_scanline, uint32_t active_line, uint32_t *line_buffer) {
    uint16_t *src = framebuf[active_line >> 1];
    uint32_t *dst = line_buffer;
    for (int x = 0; x < 320; x++) {
        uint16_t px = src[x];
        *dst++ = ((uint32_t)px << 16) | px;
    }
}

void display_init(void) {
    hstx_di_queue_init();
    video_output_init(640, 480);
    video_output_set_scanline_callback(scanline_cb);
    memset(framebuf, 0, sizeof(framebuf));
}

#endif
