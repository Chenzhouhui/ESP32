
#ifndef __LCD_H
#define __LCD_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define LCD_W 128
#define LCD_H 128

#define LCD_SOFT_SWAP_XY   0
#define LCD_SOFT_MIRROR_X  1
#define LCD_SOFT_MIRROR_Y  0

#define LCD_PIN_DC   GPIO_NUM_12
#define LCD_PIN_RST  GPIO_NUM_13

#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0

esp_err_t lcd_init(void);// 初始化 LCD 控制器与相关 GPIO/SPI
esp_err_t lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);// 设置 LCD 显示窗口（起始与结束坐标，包含边界）
esp_err_t lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);// 在指定坐标绘制单个像素点
esp_err_t lcd_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);// 使用指定颜色填充矩形区域（包含边界）
esp_err_t lcd_fill_screen(uint16_t color);// 使用指定颜色填充整屏
esp_err_t lcd_draw_bitmap_rgb565(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *data, size_t len);// 以 RGB565 大端字节序批量绘制位图


void LCD_Init(void);// 兼容旧工程接口：初始化 LCD
void LCD_Clear(u16 color);// 兼容旧工程接口：清屏
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);// 兼容旧工程接口：填充矩形
void LCD_SetWindows(u16 x_start, u16 y_start, u16 x_end, u16 y_end);// 兼容旧工程接口：设置显示窗口
void LCD_DrawPoint(u16 x, u16 y);// 兼容旧工程接口：绘制点（使用当前画笔颜色）

extern u16 POINT_COLOR;
extern u16 BACK_COLOR;

#endif
	 
	 



