
#ifndef __LCD_H
#define __LCD_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "driver/spi_master.h"


#define USER_SPI_HOST           SPI2_HOST
#define LCD_SPI_MOSI_PIN        GPIO_NUM_19
#define LCD_SPI_SCLK_PIN        GPIO_NUM_18
#define LCD_SPI_CS_PIN          GPIO_NUM_4
#define LCD_DC_PIN              GPIO_NUM_16
#define LCD_RST_PIN             GPIO_NUM_5
#define LCD_BACKLIGHT_PIN       GPIO_NUM_17
#define LCD_BACKLIGHT_ON_LEVEL  1

#define SPI_CLOCK_HZ       (30 * 1000 * 1000) 
#define LCD_W 128
#define LCD_H 128

#define LCD_SOFT_SWAP_XY   0
#define LCD_SOFT_MIRROR_X  0
#define LCD_SOFT_MIRROR_Y  0
#define LCD_X_OFFSET       2
#define LCD_Y_OFFSET       1
#define LCD_COLOR_ORDER_BGR 1
#define LCD_FONT_MIRROR_X  1

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
void lcd_fps_test_task(void);// LCD 刷新性能测试任务（持续刷新全屏并计算 FPS）

extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

#endif
	 
	 



