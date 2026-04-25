#include "GUI.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "lcdfont.h"

static inline void draw_pixel_safe(uint16_t x, uint16_t y, uint16_t color)
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

static void draw_hline_clamped(int x_start, int x_end, int y, uint16_t color)
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
        draw_pixel_safe((uint16_t)x, (uint16_t)y, color);
    }
}

void GUI_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    draw_pixel_safe(x, y, color);
}

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
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
        draw_pixel_safe((uint16_t)current_x, (uint16_t)current_y, POINT_COLOR);
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

void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_DrawLine(x1, y1, x2, y1);
    LCD_DrawLine(x2, y1, x2, y2);
    LCD_DrawLine(x2, y2, x1, y2);
    LCD_DrawLine(x1, y2, x1, y1);
}

void gui_circle(int xc, int yc, uint16_t c, int r, int fill)
{
    int x = 0;
    int y = r;
    int decision = 3 - 2 * r;

    while (x <= y) {
        if (fill) {
            for (int i = xc - x; i <= xc + x; ++i) {
                draw_pixel_safe((uint16_t)i, (uint16_t)(yc + y), c);
                draw_pixel_safe((uint16_t)i, (uint16_t)(yc - y), c);
            }
            for (int i = xc - y; i <= xc + y; ++i) {
                draw_pixel_safe((uint16_t)i, (uint16_t)(yc + x), c);
                draw_pixel_safe((uint16_t)i, (uint16_t)(yc - x), c);
            }
        } else {
            draw_pixel_safe((uint16_t)(xc + x), (uint16_t)(yc + y), c);
            draw_pixel_safe((uint16_t)(xc - x), (uint16_t)(yc + y), c);
            draw_pixel_safe((uint16_t)(xc + x), (uint16_t)(yc - y), c);
            draw_pixel_safe((uint16_t)(xc - x), (uint16_t)(yc - y), c);
            draw_pixel_safe((uint16_t)(xc + y), (uint16_t)(yc + x), c);
            draw_pixel_safe((uint16_t)(xc - y), (uint16_t)(yc + x), c);
            draw_pixel_safe((uint16_t)(xc + y), (uint16_t)(yc - x), c);
            draw_pixel_safe((uint16_t)(xc - y), (uint16_t)(yc - x), c);
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

void Draw_Circle(uint16_t x0, uint16_t y0, uint16_t fc, uint8_t r)
{
    gui_circle(x0, y0, fc, r, 0);
}

void Draw_Triangel(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_DrawLine(x0, y0, x1, y1);
    LCD_DrawLine(x1, y1, x2, y2);
    LCD_DrawLine(x2, y2, x0, y0);
}

void Fill_Triangel(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
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

static void draw_char_bitmap_scaled(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, char ch, uint8_t size, uint8_t mode)
{
    if (ch < 32 || ch > 126) {
        ch = '?';
    }

    uint8_t index = (uint8_t)(ch - 32);
    const uint8_t *bitmap = NULL;
    uint8_t bitmap_rows = 0;
    uint8_t bitmap_cols = 0;
    uint8_t scale = 1;

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

    for (uint8_t row = 0; row < bitmap_rows; ++row) {
        uint8_t bits = bitmap[row];
        for (uint8_t col = 0; col < bitmap_cols; ++col) {
#if LCD_FONT_MIRROR_X
            bool pixel_on = (bits & (1U << col)) != 0;
#else
            bool pixel_on = (bitmap_cols == 8) ? (bits & (0x80 >> col)) != 0 : (bits & (0x20 >> col)) != 0;
#endif
            for (uint8_t sy = 0; sy < scale; ++sy) {
                for (uint8_t sx = 0; sx < scale; ++sx) {
                    uint16_t px = (uint16_t)(x + col * scale + sx);
                    uint16_t py = (uint16_t)(y + row * scale + sy);
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

void LCD_ShowChar(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t num, uint8_t size, uint8_t mode)
{
    draw_char_bitmap_scaled(x, y, fc, bc, (char)num, size, mode);
}

void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size)
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
    Show_Str(x, y, POINT_COLOR, BACK_COLOR, (uint8_t *)buffer, size, 0);
}

void LCD_Show2Num(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint8_t size, uint8_t mode)
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
    Show_Str(x, y, POINT_COLOR, BACK_COLOR, (uint8_t *)buffer, size, mode);
}

void LCD_ShowString(uint16_t x, uint16_t y, uint8_t size, uint8_t *p, uint8_t mode)
{
    Show_Str(x, y, POINT_COLOR, BACK_COLOR, p, size, mode);
}

void GUI_DrawFont16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode)
{
    Show_Str(x, y, fc, bc, s, 16, mode);
}

void GUI_DrawFont24(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode)
{
    Show_Str(x, y, fc, bc, s, 24, mode);
}

void GUI_DrawFont32(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode)
{
    Show_Str(x, y, fc, bc, s, 32, mode);
}

void Show_Str(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t mode)
{
    if (str == NULL) {
        return;
    }

    uint16_t cursor_x = x;
    uint16_t step;
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

void Gui_StrCenter(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t mode)
{
    if (str == NULL) {
        return;
    }

    size_t length = strlen((const char *)str);
    uint16_t step;
    if (size == 32) {
        step = 16;
    } else if (size == 24) {
        step = 12;
    } else if (size == 16) {
        step = 8;
    } else {
        step = 6;
    }
    uint16_t total_width = (uint16_t)(length * step);
    uint16_t start_x = (x > total_width / 2) ? (x - total_width / 2) : 0;

    Show_Str(start_x, y, fc, bc, str, size, mode);
}

void Gui_Drawbmp16Ex(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const unsigned char *p)
{
    if (p == NULL) {
        return;
    }

    uint32_t offset = 0;
    for (uint16_t row = 0; row < height; ++row) {
        for (uint16_t col = 0; col < width; ++col) {
            uint16_t color = ((uint16_t)p[offset] << 8) | p[offset + 1];
            draw_pixel_safe(x + col, y + row, color);
            offset += 2;
        }
    }
}

void Gui_Drawbmp16(uint16_t x, uint16_t y, const unsigned char *p)
{
    Gui_Drawbmp16Ex(x, y, 40, 40, p);
}


void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t color, uint8_t r)
{
    Draw_Circle(x0, y0, color, r);
}

void LCD_DrawCircleEx(int xc, int yc, uint16_t color, int r, int fill)
{
    gui_circle(xc, yc, color, r, fill);
}

void LCD_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    Draw_Triangel(x0, y0, x1, y1, x2, y2);
}

void LCD_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    Fill_Triangel(x0, y0, x1, y1, x2, y2);
}

void LCD_ShowStr(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t mode)
{
    Show_Str(x, y, fc, bc, str, size, mode);
}

void LCD_ShowStringCenter(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *str, uint8_t size, uint8_t  mode)
{
    Gui_StrCenter(x, y, fc, bc, str, size, mode);
}

void LCD_DrawFont16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode)
{
    GUI_DrawFont16(x, y, fc, bc, s, mode);
}

void LCD_DrawFont24(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode)
{
    GUI_DrawFont24(x, y, fc, bc, s, mode);
}

void LCD_DrawFont32(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s, uint8_t mode)
{
    GUI_DrawFont32(x, y, fc, bc, s, mode);
}

void LCD_DrawBmp16Ex(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const unsigned char *p)
{
    Gui_Drawbmp16Ex(x, y, width, height, p);
}

void LCD_DrawBmp16(uint16_t x, uint16_t y, const unsigned char *p)
{
    Gui_Drawbmp16(x, y, p);
}
