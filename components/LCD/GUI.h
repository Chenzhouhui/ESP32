#ifndef __GUI_H__
#define __GUI_H__

#include "lcd.h"


void GUI_DrawPoint(u16 x, u16 y, u16 color);// 绘制单个像素点
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);// 绘制直线
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);// 绘制空心矩形
void LCD_DrawFillRectangle(u16 x1, u16 y1, u16 x2, u16 y2);// 绘制实心矩形（使用当前画笔颜色）
void Draw_Circle(u16 x0, u16 y0, u16 fc, u8 r);// 绘制空心圆
void gui_circle(int xc, int yc, u16 c, int r, int fill);// 圆绘制底层接口（支持空心/实心）
void Draw_Triangel(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2);// 绘制空心三角形
void Fill_Triangel(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2);// 绘制实心三角形

void LCD_ShowChar(u16 x, u16 y, u16 fc, u16 bc, u8 num, u8 size, u8 mode);// 显示单个字符
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size);// 显示无符号整数（按长度右对齐）
void LCD_Show2Num(u16 x, u16 y, u16 num, u8 len, u8 size, u8 mode);// 显示无符号整数（不足位前补 0）
void LCD_ShowString(u16 x, u16 y, u8 size, u8 *p, u8 mode);// 显示字符串

void GUI_DrawFont16(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode);// 16 号字体字符串绘制接口
void GUI_DrawFont24(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode);// 24 号字体字符串绘制接口
void GUI_DrawFont32(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode);// 32 号字体字符串绘制接口
void Show_Str(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode);// 通用字符串绘制函数
void Gui_StrCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode);// 居中显示字符串


void Gui_Drawbmp16Ex(u16 x, u16 y, u16 width, u16 height, const unsigned char *p);// 按指定宽高绘制 16 位位图
void Gui_Drawbmp16(u16 x, u16 y, const unsigned char *p);// 绘制默认尺寸 16 位位图
void LCD_GUI_TestPattern(void);// LCD 验屏图案绘制

// ===== LCD 风格别名接口（推荐日常使用） =====
void LCD_DrawCircle(u16 x0, u16 y0, u16 color, u8 r);// 绘制空心圆
void LCD_DrawCircleEx(int xc, int yc, u16 color, int r, int fill);// 绘制圆（支持空心/实心）
void LCD_DrawTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2);// 绘制空心三角形
void LCD_FillTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2);// 绘制实心三角形
void LCD_ShowStr(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode);// 通用字符串显示
void LCD_ShowStringCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode);// 居中字符串显示
void LCD_DrawFont16(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode);// 16号字体绘制
void LCD_DrawFont24(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode);// 24号字体绘制
void LCD_DrawFont32(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode);// 32号字体绘制
void LCD_DrawBmp16Ex(u16 x, u16 y, u16 width, u16 height, const unsigned char *p);// 指定尺寸位图绘制
void LCD_DrawBmp16(u16 x, u16 y, const unsigned char *p);// 默认尺寸位图绘制

#endif
