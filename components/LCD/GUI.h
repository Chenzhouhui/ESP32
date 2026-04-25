#ifndef __GUI_H__
#define __GUI_H__

#include "lcd.h"


void GUI_DrawPoint(uint16_t x, uint16_t y, uint16_t color);// 绘制单个像素点
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);// 绘制直线
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);// 绘制空心矩形
void LCD_DrawFillRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);// 绘制实心矩形（使用当前画笔颜色）
void Draw_Circle(uint16_t x0, uint16_t y0, uint16_t fc, uint8_t r);// 绘制空心圆
void gui_circle(int xc, int yc, uint16_t c, int r, int fill);// 圆绘制底层接口（支持空心/实心）
void Draw_Triangel(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);// 绘制空心三角形
void Fill_Triangel(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);// 绘制实心三角形

void LCD_ShowChar(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t num, uint8_t size, uint8_t mode);// 显示单个字符
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);// 显示无符号整数（按长度右对齐）
void LCD_Show2Num(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint8_t size, uint8_t mode);// 显示无符号整数（不足位前补 0）
void LCD_ShowString(uint16_t x, uint16_t y, uint8_t size, uint8_t *p, uint8_t mode);// 显示字符串

void GUI_DrawFont16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode);// 16 号字体字符串绘制接口
void GUI_DrawFont24(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode);// 24 号字体字符串绘制接口
void GUI_DrawFont32(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode);// 32 号字体字符串绘制接口
void Show_Str(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t mode);// 通用字符串绘制函数
void Gui_StrCenter(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t mode);// 居中显示字符串


void Gui_Drawbmp16Ex(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const unsigned char *p);// 按指定宽高绘制 16 位位图
void Gui_Drawbmp16(uint16_t x, uint16_t y, const unsigned char *p);// 绘制默认尺寸 16 位位图

// ===== LCD 风格别名接口（推荐日常使用） =====
void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t color, uint8_t r);// 绘制空心圆
void LCD_DrawCircleEx(int xc, int yc, uint16_t color, int r, int fill);// 绘制圆（支持空心/实心）
void LCD_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);// 绘制空心三角形
void LCD_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);// 绘制实心三角形
void LCD_ShowStr(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t mode);// 通用字符串显示
void LCD_ShowStringCenter(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t mode);// 居中字符串显示
void LCD_DrawFont16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode);// 16号字体绘制
void LCD_DrawFont24(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode);// 24号字体绘制
void LCD_DrawFont32(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode);// 32号字体绘制
void LCD_DrawBmp16Ex(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const unsigned char *p);// 指定尺寸位图绘制
void LCD_DrawBmp16(uint16_t x, uint16_t y, const unsigned char *p);// 默认尺寸位图绘制

#endif
