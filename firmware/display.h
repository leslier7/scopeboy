#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

// RGB565 color constants (mirrors ILI9340 values)
#define ILI9340_BLACK   0x0000
#define ILI9340_BLUE    0x001F
#define ILI9340_RED     0xF800
#define ILI9340_GREEN   0x07E0
#define ILI9340_CYAN    0x07FF
#define ILI9340_MAGENTA 0xF81F
#define ILI9340_YELLOW  0xFFE0
#define ILI9340_WHITE   0xFFFF

void display_init(void);
void display_fillScreen(uint16_t color);
void display_fillRect(short x, short y, short w, short h, uint16_t color);
void display_drawPixel(short x, short y, uint16_t color);
void display_drawLine(short x0, short y0, short x1, short y1, uint16_t color);
void display_drawFastHLine(short x, short y, short w, uint16_t color);
void display_drawFastVLine(short x, short y, short h, uint16_t color);
void display_setCursor(short x, short y);
void display_setTextColor(uint16_t color);
void display_setTextSize(uint8_t size);
void display_writeString(char *str);

#endif
