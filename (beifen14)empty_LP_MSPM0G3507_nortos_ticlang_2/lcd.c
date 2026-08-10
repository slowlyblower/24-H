/*
 * lcd.c - ST7789 170x320 SPI 驱动
 *
 * 引脚:
 *   PB9=SCL  PB8=SDA  PB10=RES  PB11=DC  PB14=CS  PB26=BLK
 */
#include "lcd.h"
#include "ti_msp_dl_config.h"

/* ---- 引脚 ---- */
#define SCL_PORT   GPIOB
#define SCL_PIN    DL_GPIO_PIN_9
#define SCL_IOMUX  IOMUX_PINCM26

#define SDA_PORT   GPIOB
#define SDA_PIN    DL_GPIO_PIN_8
#define SDA_IOMUX  IOMUX_PINCM25

#define RES_PORT   GPIOB
#define RES_PIN    DL_GPIO_PIN_10
#define RES_IOMUX  IOMUX_PINCM27

#define DC_PORT    GPIOB
#define DC_PIN     DL_GPIO_PIN_11
#define DC_IOMUX   IOMUX_PINCM28

#define CS_PORT    GPIOB
#define CS_PIN     DL_GPIO_PIN_14
#define CS_IOMUX   IOMUX_PINCM31

#define BLK_PORT   GPIOB
#define BLK_PIN    DL_GPIO_PIN_26
#define BLK_IOMUX  IOMUX_PINCM57

/* 屏幕分辨率 (240x240) */
#define SW  240
#define SH  240

/* 内部状态 */
static uint16_t g_fg = LCD_WHITE, g_bg = LCD_BLACK;
static uint16_t g_cx = 0, g_cy = 0;

static void delay_us(uint32_t us) { delay_cycles(us * 32); }

/* ---- SPI 写 1 字节 ---- */
static void spi_write(uint8_t d)
{
    for (uint8_t i = 0; i < 8; i++) {
        DL_GPIO_clearPins(SCL_PORT, SCL_PIN);
        if (d & 0x80) DL_GPIO_setPins(SDA_PORT, SDA_PIN);
        else          DL_GPIO_clearPins(SDA_PORT, SDA_PIN);
        DL_GPIO_setPins(SCL_PORT, SCL_PIN);
        d <<= 1;
    }
}

static void lcd_cmd(uint8_t c)
{
    DL_GPIO_clearPins(CS_PORT, CS_PIN);
    DL_GPIO_clearPins(DC_PORT, DC_PIN);
    spi_write(c);
    DL_GPIO_setPins(CS_PORT, CS_PIN);
}

static void lcd_data(uint8_t d)
{
    DL_GPIO_clearPins(CS_PORT, CS_PIN);
    DL_GPIO_setPins(DC_PORT, DC_PIN);
    spi_write(d);
    DL_GPIO_setPins(CS_PORT, CS_PIN);
}

static void lcd_data16(uint16_t d)
{
    lcd_data((uint8_t)(d >> 8));
    lcd_data((uint8_t)(d & 0xFF));
}

/* 设显示窗口 (ST7789内部240x320, 170宽需X偏移35) */
static void lcd_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_cmd(0x2A); lcd_data16(x0); lcd_data16(x1);
    lcd_cmd(0x2B); lcd_data16(y0); lcd_data16(y1);
    lcd_cmd(0x2C);
}

/* ---- 初始化 ---- */
void lcd_init(void)
{
    /* GPIO */
    DL_GPIO_initDigitalOutput(SCL_IOMUX);
    DL_GPIO_setPins(SCL_PORT, SCL_PIN); DL_GPIO_enableOutput(SCL_PORT, SCL_PIN);
    DL_GPIO_initDigitalOutput(SDA_IOMUX);
    DL_GPIO_setPins(SDA_PORT, SDA_PIN); DL_GPIO_enableOutput(SDA_PORT, SDA_PIN);
    DL_GPIO_initDigitalOutput(RES_IOMUX);
    DL_GPIO_setPins(RES_PORT, RES_PIN); DL_GPIO_enableOutput(RES_PORT, RES_PIN);
    DL_GPIO_initDigitalOutput(DC_IOMUX);
    DL_GPIO_setPins(DC_PORT, DC_PIN);   DL_GPIO_enableOutput(DC_PORT, DC_PIN);
    DL_GPIO_initDigitalOutput(CS_IOMUX);
    DL_GPIO_setPins(CS_PORT, CS_PIN);   DL_GPIO_enableOutput(CS_PORT, CS_PIN);
    DL_GPIO_initDigitalOutput(BLK_IOMUX);
    DL_GPIO_setPins(BLK_PORT, BLK_PIN); DL_GPIO_enableOutput(BLK_PORT, BLK_PIN);  /* 背光开 */

    /* 软件复位 (处理MCU热重启时LCD未掉电的情况) */
    lcd_cmd(0x01);  delay_us(150000);  /* SW reset */

    /* 硬件复位 */
    DL_GPIO_clearPins(RES_PORT, RES_PIN); delay_us(20000);
    DL_GPIO_setPins(RES_PORT, RES_PIN);   delay_us(150000);

    /* ST7789 初始化序列 */
    lcd_cmd(0x11);  delay_us(120000);          /* 退出睡眠 */
    lcd_cmd(0x36);  lcd_data(0x00);             /* MADCTL: 从上到下,从左到右 */
    lcd_cmd(0x3A);  lcd_data(0x05);             /* 16bit/像素 */
    lcd_cmd(0xB2);  lcd_data(0x0C);lcd_data(0x0C);lcd_data(0x00);lcd_data(0x33);lcd_data(0x33);
    lcd_cmd(0xB7);  lcd_data(0x35);             /* VGH/VGL */
    lcd_cmd(0xBB);  lcd_data(0x19);             /* VCOM */
    lcd_cmd(0xC0);  lcd_data(0x2C);             /* LCM */
    lcd_cmd(0xC2);  lcd_data(0x01);
    lcd_cmd(0xC3);  lcd_data(0x12);
    lcd_cmd(0xC4);  lcd_data(0x20);
    lcd_cmd(0xC6);  lcd_data(0x0F);
    lcd_cmd(0xD0);  lcd_data(0xA4);lcd_data(0xA1);
    /* Gamma */
    lcd_cmd(0xE0);  lcd_data(0xD0);lcd_data(0x04);lcd_data(0x0D);lcd_data(0x11);
    lcd_data(0x13);lcd_data(0x2B);lcd_data(0x3F);lcd_data(0x54);
    lcd_data(0x4C);lcd_data(0x18);lcd_data(0x0D);lcd_data(0x0B);
    lcd_data(0x1F);lcd_data(0x23);
    lcd_cmd(0xE1);  lcd_data(0xD0);lcd_data(0x04);lcd_data(0x0C);lcd_data(0x11);
    lcd_data(0x13);lcd_data(0x2C);lcd_data(0x3F);lcd_data(0x44);
    lcd_data(0x51);lcd_data(0x2F);lcd_data(0x1F);lcd_data(0x1F);
    lcd_data(0x20);lcd_data(0x23);
    lcd_cmd(0x29);                              /* 显示开 */

    lcd_clear(LCD_BLACK);
}

/* ---- 清屏 ---- */
void lcd_clear(uint16_t color)
{
    lcd_window(0, 0, SW - 1, SH - 1);
    DL_GPIO_clearPins(CS_PORT, CS_PIN);
    DL_GPIO_setPins(DC_PORT, DC_PIN);
    for (uint32_t i = 0; i < (uint32_t)SW * SH; i++)
        lcd_data16(color);
    DL_GPIO_setPins(CS_PORT, CS_PIN);
    g_cx = 0; g_cy = 0;
}

/* ---- 设坐标: row=行号(0起), col=列像素(0起) ---- */
void lcd_set_cursor(uint16_t row, uint16_t col) { g_cx = col; g_cy = row * 16; }

/* ---- 8x16 字库 (ASCII 32-126) ---- */
#include "font_8x16.h"  /* 放在外部 */

void lcd_putc(char c)
{
    if (c < 32 || c > 126) c = ' ';
    uint8_t idx = (uint8_t)(c - 32);

    if (g_cx + 8 > SW) { g_cx = 0; g_cy += 16; }
    if (g_cy + 16 > SH) { g_cy = 0; lcd_clear(g_bg); }

    lcd_window(g_cx, g_cy, g_cx + 7, g_cy + 15);
    DL_GPIO_clearPins(CS_PORT, CS_PIN);
    DL_GPIO_setPins(DC_PORT, DC_PIN);
    for (uint8_t r = 0; r < 16; r++) {
        uint8_t line = g_font_8x16[idx][r];
        for (uint8_t b = 0; b < 8; b++) {
            lcd_data16((line & 0x80) ? g_fg : g_bg);
            line <<= 1;
        }
    }
    DL_GPIO_setPins(CS_PORT, CS_PIN);
    g_cx += 8;
}

void lcd_puts(const char *s) { while (*s) lcd_putc(*s++); }

void lcd_print_int(int32_t n)
{
    char b[12]; int i = 0;
    if (n < 0) { lcd_putc('-'); n = -n; }
    if (!n) { lcd_putc('0'); return; }
    while (n) { b[i++] = '0' + (n % 10); n /= 10; }
    while (i) lcd_putc(b[--i]);
}

void lcd_set_color(uint16_t fg, uint16_t bg) { g_fg = fg; g_bg = bg; }
