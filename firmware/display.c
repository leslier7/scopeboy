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
// HDMI path — framebuffer + stubs (implementations TODO)
// ============================================================

#include "pico_hdmi/video_output.h"
#include <string.h>

static uint16_t framebuf[240][320];

void display_init(void)                                                           {}
void display_fillScreen(uint16_t color)                                           {}
void display_fillRect(short x, short y, short w, short h, uint16_t color)        {}
void display_drawPixel(short x, short y, uint16_t color)                          {}
void display_drawLine(short x0, short y0, short x1, short y1, uint16_t color)    {}
void display_drawFastHLine(short x, short y, short w, uint16_t color)             {}
void display_drawFastVLine(short x, short y, short h, uint16_t color)             {}
void display_setCursor(short x, short y)                                          {}
void display_setTextColor(uint16_t color)                                         {}
void display_setTextSize(uint8_t size)                                            {}
void display_writeString(char *str)                                               {}

#endif
