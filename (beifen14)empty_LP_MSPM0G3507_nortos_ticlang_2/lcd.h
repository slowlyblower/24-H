/*
 * lcd.h - ST7789 1.9寸 170x320 SPI LCD
 *
 * 引脚(天猛星 FPC):
 *   PB9→SCL  PB8→SDA  PB10→RES  PB11→DC  PB14→CS  PB26→BLK
 */
#ifndef LCD_H
#define LCD_H
#include <stdint.h>

void lcd_init(void);
void lcd_clear(uint16_t color);
void lcd_set_cursor(uint16_t x, uint16_t y);
void lcd_puts(const char *s);
void lcd_putc(char c);
void lcd_print_int(int32_t n);
void lcd_set_color(uint16_t fg, uint16_t bg);

#define LCD_BLACK   0x0000
#define LCD_WHITE   0xFFFF
#define LCD_RED     0xF800
#define LCD_GREEN   0x07E0
#define LCD_BLUE    0x001F
#define LCD_YELLOW  0xFFE0

#endif
