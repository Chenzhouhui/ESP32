#include "lcd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "spi.h"

#define ST7735_CMD_SWRESET 0x01
#define ST7735_CMD_SLPOUT  0x11
#define ST7735_CMD_COLMOD  0x3A
#define ST7735_CMD_MADCTL  0x36
#define ST7735_CMD_CASET   0x2A
#define ST7735_CMD_RASET   0x2B
#define ST7735_CMD_RAMWR   0x2C
#define ST7735_CMD_DISPON  0x29

u16 POINT_COLOR = RED;
u16 BACK_COLOR = BLACK;

static bool s_lcd_initialized = false;

static inline void lcd_swap_u16(uint16_t *left, uint16_t *right)
{
    uint16_t temp = *left;
    *left = *right;
    *right = temp;
}

static inline void lcd_transform_xy(uint16_t *x, uint16_t *y)
{
#if LCD_SOFT_SWAP_XY
    lcd_swap_u16(x, y);
#endif
#if LCD_SOFT_MIRROR_X
    *x = (uint16_t)(LCD_W - 1 - *x);
#endif
#if LCD_SOFT_MIRROR_Y
    *y = (uint16_t)(LCD_H - 1 - *y);
#endif
}

static inline void lcd_dc_cmd(void)
{
    gpio_set_level(LCD_PIN_DC, 0);
}

static inline void lcd_dc_data(void)
{
    gpio_set_level(LCD_PIN_DC, 1);
}

static void lcd_write_cmd(uint8_t cmd)
{
    lcd_dc_cmd();
    spi_write_data(&cmd, 1);
}

static void lcd_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) {
        return;
    }

    lcd_dc_data();
    spi_write_data(data, len);
}

static void lcd_write_u16(uint16_t value)
{
    uint8_t buf[2] = {(uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    lcd_write_data(buf, sizeof(buf));
}

esp_err_t lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (!s_lcd_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t tx0 = x0;
    uint16_t ty0 = y0;
    uint16_t tx1 = x1;
    uint16_t ty1 = y1;

    lcd_transform_xy(&tx0, &ty0);
    lcd_transform_xy(&tx1, &ty1);

    if (tx0 > tx1) {
        lcd_swap_u16(&tx0, &tx1);
    }
    if (ty0 > ty1) {
        lcd_swap_u16(&ty0, &ty1);
    }

    lcd_write_cmd(ST7735_CMD_CASET);
    lcd_write_u16(tx0);
    lcd_write_u16(tx1);

    lcd_write_cmd(ST7735_CMD_RASET);
    lcd_write_u16(ty0);
    lcd_write_u16(ty1);

    lcd_write_cmd(ST7735_CMD_RAMWR);

    return ESP_OK;
}

esp_err_t lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_W || y >= LCD_H) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = lcd_set_window(x, y, x, y);
    if (ret != ESP_OK) {
        return ret;
    }

    lcd_write_u16(color);
    return ESP_OK;
}

esp_err_t lcd_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    if (x0 >= LCD_W || y0 >= LCD_H) {
        return ESP_ERR_INVALID_ARG;
    }

    if (x1 >= LCD_W) {
        x1 = LCD_W - 1;
    }
    if (y1 >= LCD_H) {
        y1 = LCD_H - 1;
    }

    if (x0 > x1) {
        uint16_t temp = x0;
        x0 = x1;
        x1 = temp;
    }
    if (y0 > y1) {
        uint16_t temp = y0;
        y0 = y1;
        y1 = temp;
    }

    esp_err_t ret = lcd_set_window(x0, y0, x1, y1);
    if (ret != ESP_OK) {
        return ret;
    }

    uint32_t pixels = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);

    enum { PIXELS_PER_CHUNK = 256 };
    uint8_t color_chunk[PIXELS_PER_CHUNK * 2];
    for (size_t index = 0; index < PIXELS_PER_CHUNK; ++index) {
        color_chunk[index * 2] = (uint8_t)(color >> 8);
        color_chunk[index * 2 + 1] = (uint8_t)(color & 0xFF);
    }

    while (pixels > 0) {
        size_t chunk_pixels = (pixels > PIXELS_PER_CHUNK) ? PIXELS_PER_CHUNK : (size_t)pixels;
        lcd_write_data(color_chunk, chunk_pixels * 2);
        pixels -= (uint32_t)chunk_pixels;
    }

    return ESP_OK;
}

esp_err_t lcd_fill_screen(uint16_t color)
{
    return lcd_fill_rect(0, 0, LCD_W - 1, LCD_H - 1, color);
}

esp_err_t lcd_init(void)
{
    spi_init();

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_PIN_DC) | (1ULL << LCD_PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        return ret;
    }

    gpio_set_level(LCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_write_cmd(ST7735_CMD_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    lcd_write_cmd(ST7735_CMD_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_write_cmd(ST7735_CMD_COLMOD);
    uint8_t colmod = 0x05;
    lcd_write_data(&colmod, 1);

    lcd_write_cmd(ST7735_CMD_MADCTL);
    uint8_t madctl = 0x00;
    lcd_write_data(&madctl, 1);

    lcd_write_cmd(ST7735_CMD_DISPON);
    vTaskDelay(pdMS_TO_TICKS(20));

    s_lcd_initialized = true;

    return lcd_fill_screen(BLACK);
}

void LCD_Init(void)
{
    lcd_init();
}

void LCD_Clear(u16 color)
{
    lcd_fill_screen(color);
}

void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
    lcd_fill_rect(sx, sy, ex, ey, color);
}

void LCD_SetWindows(u16 x_start, u16 y_start, u16 x_end, u16 y_end)
{
    lcd_set_window(x_start, y_start, x_end, y_end);
}

void LCD_DrawPoint(u16 x, u16 y)
{
    lcd_draw_pixel(x, y, POINT_COLOR);
}

