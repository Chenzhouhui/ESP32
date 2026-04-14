#include "GUI.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "lcdfont.h"

static inline void draw_pixel_safe(u16 x, u16 y, u16 color)
{
    if (x < LCD_W && y < LCD_H) {
        lcd_draw_pixel(x, y, color);
    }
}

static inline void swap_int(int *left, int *right)
{
    int temp = *left;
    *left = *right;
    *right = temp;
}

static void draw_hline_clamped(int x_start, int x_end, int y, u16 color)
{
    if (y < 0 || y >= LCD_H) {
        return;
    }
    if (x_start > x_end) {
        swap_int(&x_start, &x_end);
    }
    if (x_end < 0 || x_start >= LCD_W) {
        return;
    }

    if (x_start < 0) {
        x_start = 0;
    }
    if (x_end >= LCD_W) {
        x_end = LCD_W - 1;
    }

    for (int x = x_start; x <= x_end; ++x) {
        draw_pixel_safe((u16)x, (u16)y, color);
    }
}

void GUI_DrawPoint(u16 x, u16 y, u16 color)
{
    draw_pixel_safe(x, y, color);
}

void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
    int dx = (int)x2 - (int)x1;
    int dy = (int)y2 - (int)y1;
    int step_x = (dx >= 0) ? 1 : -1;
    int step_y = (dy >= 0) ? 1 : -1;
    dx = step_x * dx;
    dy = step_y * dy;

    int error = ((dx > dy) ? dx : -dy) / 2;
    int current_x = x1;
    int current_y = y1;

    while (1) {
        draw_pixel_safe((u16)current_x, (u16)current_y, POINT_COLOR);
        if (current_x == x2 && current_y == y2) {
            break;
        }

        int error2 = error;
        if (error2 > -dx) {
            error -= dy;
            current_x += step_x;
        }
        if (error2 < dy) {
            error += dx;
            current_y += step_y;
        }
    }
}

void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
    LCD_DrawLine(x1, y1, x2, y1);
    LCD_DrawLine(x2, y1, x2, y2);
    LCD_DrawLine(x2, y2, x1, y2);
    LCD_DrawLine(x1, y2, x1, y1);
}

void LCD_DrawFillRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
    LCD_Fill(x1, y1, x2, y2, POINT_COLOR);
}

void gui_circle(int xc, int yc, u16 c, int r, int fill)
{
    int x = 0;
    int y = r;
    int decision = 3 - 2 * r;

    while (x <= y) {
        if (fill) {
            for (int i = xc - x; i <= xc + x; ++i) {
                draw_pixel_safe((u16)i, (u16)(yc + y), c);
                draw_pixel_safe((u16)i, (u16)(yc - y), c);
            }
            for (int i = xc - y; i <= xc + y; ++i) {
                draw_pixel_safe((u16)i, (u16)(yc + x), c);
                draw_pixel_safe((u16)i, (u16)(yc - x), c);
            }
        } else {
            draw_pixel_safe((u16)(xc + x), (u16)(yc + y), c);
            draw_pixel_safe((u16)(xc - x), (u16)(yc + y), c);
            draw_pixel_safe((u16)(xc + x), (u16)(yc - y), c);
            draw_pixel_safe((u16)(xc - x), (u16)(yc - y), c);
            draw_pixel_safe((u16)(xc + y), (u16)(yc + x), c);
            draw_pixel_safe((u16)(xc - y), (u16)(yc + x), c);
            draw_pixel_safe((u16)(xc + y), (u16)(yc - x), c);
            draw_pixel_safe((u16)(xc - y), (u16)(yc - x), c);
        }

        if (decision < 0) {
            decision += 4 * x + 6;
        } else {
            decision += 4 * (x - y) + 10;
            --y;
        }
        ++x;
    }
}

void Draw_Circle(u16 x0, u16 y0, u16 fc, u8 r)
{
    gui_circle(x0, y0, fc, r, 0);
}

void Draw_Triangel(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2)
{
    LCD_DrawLine(x0, y0, x1, y1);
    LCD_DrawLine(x1, y1, x2, y2);
    LCD_DrawLine(x2, y2, x0, y0);
}

void Fill_Triangel(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2)
{
    int ax = x0;
    int ay = y0;
    int bx = x1;
    int by = y1;
    int cx = x2;
    int cy = y2;

    if (ay > by) {
        swap_int(&ax, &bx);
        swap_int(&ay, &by);
    }
    if (ay > cy) {
        swap_int(&ax, &cx);
        swap_int(&ay, &cy);
    }
    if (by > cy) {
        swap_int(&bx, &cx);
        swap_int(&by, &cy);
    }

    if (ay == cy) {
        int min_x = ax;
        int max_x = ax;
        if (bx < min_x) min_x = bx;
        if (cx < min_x) min_x = cx;
        if (bx > max_x) max_x = bx;
        if (cx > max_x) max_x = cx;
        draw_hline_clamped(min_x, max_x, ay, POINT_COLOR);
        return;
    }

    for (int y = ay; y <= cy; ++y) {
        if (y < 0 || y >= LCD_H) {
            continue;
        }

        float alpha = (cy == ay) ? 0.0f : (float)(y - ay) / (float)(cy - ay);
        float beta;

        int x_start = ax + (int)((cx - ax) * alpha);
        int x_end;

        if (y <= by) {
            beta = (by == ay) ? 0.0f : (float)(y - ay) / (float)(by - ay);
            x_end = ax + (int)((bx - ax) * beta);
        } else {
            beta = (cy == by) ? 0.0f : (float)(y - by) / (float)(cy - by);
            x_end = bx + (int)((cx - bx) * beta);
        }

        draw_hline_clamped(x_start, x_end, y, POINT_COLOR);
    }
}

static void draw_char_bitmap_scaled(u16 x, u16 y, u16 fc, u16 bc, char ch, u8 size, u8 mode)
{
    if (ch < 32 || ch > 126) {
        ch = '?';
    }

    u8 index = (u8)(ch - 32);
    const u8 *bitmap = NULL;
    u8 bitmap_rows = 0;
    u8 bitmap_cols = 0;
    u8 scale = 1;

    if (size == 32) {
        bitmap = asc2_1608[index];
        bitmap_rows = 16;
        bitmap_cols = 8;
        scale = 2;
    } else if (size == 24) {
        bitmap = asc2_1206[index];
        bitmap_rows = 12;
        bitmap_cols = 6;
        scale = 2;
    } else if (size == 16) {
        bitmap = asc2_1608[index];
        bitmap_rows = 16;
        bitmap_cols = 8;
        scale = 1;
    } else {
        bitmap = asc2_1206[index];
        bitmap_rows = 12;
        bitmap_cols = 6;
        scale = 1;
    }

    for (u8 row = 0; row < bitmap_rows; ++row) {
        u8 bits = bitmap[row];
        for (u8 col = 0; col < bitmap_cols; ++col) {
            bool pixel_on = (bitmap_cols == 8) ? (bits & (0x80 >> col)) : (bits & (0x20 >> col));
            for (u8 sy = 0; sy < scale; ++sy) {
                for (u8 sx = 0; sx < scale; ++sx) {
                    u16 px = (u16)(x + col * scale + sx);
                    u16 py = (u16)(y + row * scale + sy);
                    if (pixel_on) {
                        draw_pixel_safe(px, py, fc);
                    } else if (mode == 0) {
                        draw_pixel_safe(px, py, bc);
                    }
                }
            }
        }
    }
}

void LCD_ShowChar(u16 x, u16 y, u16 fc, u16 bc, u8 num, u8 size, u8 mode)
{
    draw_char_bitmap_scaled(x, y, fc, bc, (char)num, size, mode);
}

void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size)
{
    char buffer[16];
    int width = len;
    if (width < 1) {
        width = 1;
    }
    if (width > (int)sizeof(buffer) - 1) {
        width = (int)sizeof(buffer) - 1;
    }

    snprintf(buffer, sizeof(buffer), "%*lu", width, (unsigned long)num);
    Show_Str(x, y, POINT_COLOR, BACK_COLOR, (u8 *)buffer, size, 0);
}

void LCD_Show2Num(u16 x, u16 y, u16 num, u8 len, u8 size, u8 mode)
{
    char buffer[16];
    int width = len;
    if (width < 1) {
        width = 1;
    }
    if (width > (int)sizeof(buffer) - 1) {
        width = (int)sizeof(buffer) - 1;
    }

    snprintf(buffer, sizeof(buffer), "%0*u", width, num);
    Show_Str(x, y, POINT_COLOR, BACK_COLOR, (u8 *)buffer, size, mode);
}

void LCD_ShowString(u16 x, u16 y, u8 size, u8 *p, u8 mode)
{
    Show_Str(x, y, POINT_COLOR, BACK_COLOR, p, size, mode);
}

void GUI_DrawFont16(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode)
{
    Show_Str(x, y, fc, bc, s, 16, mode);
}

void GUI_DrawFont24(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode)
{
    Show_Str(x, y, fc, bc, s, 24, mode);
}

void GUI_DrawFont32(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode)
{
    Show_Str(x, y, fc, bc, s, 32, mode);
}

void Show_Str(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode)
{
    if (str == NULL) {
        return;
    }

    u16 cursor_x = x;
    u16 step;
    if (size == 32) {
        step = 16;
    } else if (size == 24) {
        step = 12;
    } else if (size == 16) {
        step = 8;
    } else {
        step = 6;
    }

    size_t length = strlen((const char *)str);

#if LCD_SOFT_MIRROR_X
    for (size_t i = length; i > 0; --i) {
        draw_char_bitmap_scaled(cursor_x, y, fc, bc, (char)str[i - 1], size, mode);
        cursor_x += step;
        if (cursor_x + step >= LCD_W) {
            break;
        }
    }
#else
    for (size_t i = 0; i < length; ++i) {
        draw_char_bitmap_scaled(cursor_x, y, fc, bc, (char)str[i], size, mode);
        cursor_x += step;
        if (cursor_x + step >= LCD_W) {
            break;
        }
    }
#endif
}

void Gui_StrCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode)
{
    if (str == NULL) {
        return;
    }

    size_t length = strlen((const char *)str);
    u16 step;
    if (size == 32) {
        step = 16;
    } else if (size == 24) {
        step = 12;
    } else if (size == 16) {
        step = 8;
    } else {
        step = 6;
    }
    u16 total_width = (u16)(length * step);
    u16 start_x = (x > total_width / 2) ? (x - total_width / 2) : 0;

    Show_Str(start_x, y, fc, bc, str, size, mode);
}

void Gui_Drawbmp16Ex(u16 x, u16 y, u16 width, u16 height, const unsigned char *p)
{
    if (p == NULL) {
        return;
    }

    uint32_t offset = 0;
    for (u16 row = 0; row < height; ++row) {
        for (u16 col = 0; col < width; ++col) {
            u16 color = ((u16)p[offset] << 8) | p[offset + 1];
            draw_pixel_safe(x + col, y + row, color);
            offset += 2;
        }
    }
}

void Gui_Drawbmp16(u16 x, u16 y, const unsigned char *p)
{
    Gui_Drawbmp16Ex(x, y, 40, 40, p);
}

void LCD_GUI_TestPattern(void)
{
    LCD_Clear(BLACK);

    POINT_COLOR = RED;
    LCD_DrawRectangle(2, 2, LCD_W - 3, LCD_H - 3);

    POINT_COLOR = GREEN;
    LCD_DrawLine(0, 0, LCD_W - 1, LCD_H - 1);
    LCD_DrawLine(LCD_W - 1, 0, 0, LCD_H - 1);

    POINT_COLOR = BLUE;
    Fill_Triangel(15, 100, 60, 40, 110, 110);

    GUI_DrawFont16(5, 5, YELLOW, BLACK, (u8 *)"ESP32 LCD", 0);
    GUI_DrawFont24(5, 24, CYAN, BLACK, (u8 *)"GUI 24", 0);
    GUI_DrawFont32(5, 52, MAGENTA, BLACK, (u8 *)"32", 0);
}

void LCD_DrawCircle(u16 x0, u16 y0, u16 color, u8 r)
{
    Draw_Circle(x0, y0, color, r);
}

void LCD_DrawCircleEx(int xc, int yc, u16 color, int r, int fill)
{
    gui_circle(xc, yc, color, r, fill);
}

void LCD_DrawTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2)
{
    Draw_Triangel(x0, y0, x1, y1, x2, y2);
}

void LCD_FillTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2)
{
    Fill_Triangel(x0, y0, x1, y1, x2, y2);
}

void LCD_ShowStr(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode)
{
    Show_Str(x, y, fc, bc, str, size, mode);
}

void LCD_ShowStringCenter(u16 x, u16 y, u16 fc, u16 bc, u8 *str, u8 size, u8 mode)
{
    Gui_StrCenter(x, y, fc, bc, str, size, mode);
}

void LCD_DrawFont16(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode)
{
    GUI_DrawFont16(x, y, fc, bc, s, mode);
}

void LCD_DrawFont24(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode)
{
    GUI_DrawFont24(x, y, fc, bc, s, mode);
}

void LCD_DrawFont32(u16 x, u16 y, u16 fc, u16 bc, u8 *s, u8 mode)
{
    GUI_DrawFont32(x, y, fc, bc, s, mode);
}

void LCD_DrawBmp16Ex(u16 x, u16 y, u16 width, u16 height, const unsigned char *p)
{
    Gui_Drawbmp16Ex(x, y, width, height, p);
}

void LCD_DrawBmp16(u16 x, u16 y, const unsigned char *p)
{
    Gui_Drawbmp16(x, y, p);
}
