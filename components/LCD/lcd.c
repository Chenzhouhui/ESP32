#include "lcd.h"
#include "GUI.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

#define ST7735_CMD_SWRESET 0x01
#define ST7735_CMD_SLPOUT  0x11
#define ST7735_CMD_COLMOD  0x3A
#define ST7735_CMD_MADCTL  0x36
#define ST7735_CMD_CASET   0x2A
#define ST7735_CMD_RASET   0x2B
#define ST7735_CMD_RAMWR   0x2C
#define ST7735_CMD_DISPON  0x29
/************************************************************** */
static spi_device_handle_t spi_handle;
esp_err_t spi_init(void)
{
    spi_bus_config_t spi_init_struct = {
        .mosi_io_num = LCD_SPI_MOSI_PIN,
        .sclk_io_num = LCD_SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 16 * 1024,
        .flags = 0, // No special flags
        .isr_cpu_id = 0, // Use default CPU for SPI ISR
        .intr_flags = 0 // No special interrupt flags
    };
    esp_err_t ret = spi_bus_initialize(USER_SPI_HOST, &spi_init_struct, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        return ret;
    }

    spi_device_interface_config_t spi_device_config = {
        .clock_speed_hz = SPI_CLOCK_HZ, // Set the SPI clock speed
        .mode = 0, // SPI mode 0
        .queue_size = 8,
        .spics_io_num = LCD_SPI_CS_PIN,
        .pre_cb = NULL,
    };
    ret = spi_bus_add_device(USER_SPI_HOST, &spi_device_config, &spi_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;

}

void spi_write_data(const uint8_t *data, size_t len)
{
    spi_transaction_t transaction = {
        .length = len * 8,
        .flags = 0,
        .rx_buffer = NULL,
        .tx_buffer = data
    };
    spi_device_transmit(spi_handle, &transaction);
}
void spi_read_data(uint8_t *rxdata, size_t len)
{
    spi_transaction_t transaction = {
        .length = len * 8,
        .flags = 0,
        .rx_buffer = rxdata,
        .tx_buffer = NULL
    };
    spi_device_transmit(spi_handle, &transaction);
}
/*************************************************************** */
uint16_t POINT_COLOR = RED;
uint16_t BACK_COLOR = BLACK;

static bool s_lcd_initialized = false;

static inline void lcd_swap_uint16_t(uint16_t *left, uint16_t *right)
{
    uint16_t temp = *left;
    *left = *right;
    *right = temp;
}

static inline void lcd_transform_xy(uint16_t *x, uint16_t *y)
{
#if LCD_SOFT_SWAP_XY
    lcd_swap_uint16_t(x, y);
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
    gpio_set_level(LCD_DC_PIN, 0);
}

static inline void lcd_dc_data(void)
{
    gpio_set_level(LCD_DC_PIN, 1);
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
        lcd_swap_uint16_t(&tx0, &tx1);
    }
    if (ty0 > ty1) {
        lcd_swap_uint16_t(&ty0, &ty1);
    }

    tx0 = (uint16_t)(tx0 + LCD_X_OFFSET);
    tx1 = (uint16_t)(tx1 + LCD_X_OFFSET);
    ty0 = (uint16_t)(ty0 + LCD_Y_OFFSET);
    ty1 = (uint16_t)(ty1 + LCD_Y_OFFSET);

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
        lcd_swap_uint16_t(&x0, &x1);
    }
    if (y0 > y1) {
        lcd_swap_uint16_t(&y0, &y1);
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
    esp_err_t ret = spi_init();
    if (ret != ESP_OK) {
        return ret;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_DC_PIN) | (1ULL << LCD_RST_PIN) | (1ULL << LCD_BACKLIGHT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        return ret;
    }

    gpio_set_level(LCD_BACKLIGHT_PIN, !LCD_BACKLIGHT_ON_LEVEL);

    gpio_set_level(LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_write_cmd(ST7735_CMD_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    lcd_write_cmd(ST7735_CMD_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_write_cmd(ST7735_CMD_COLMOD);
    uint8_t colmod = 0x05;
    lcd_write_data(&colmod, 1);

    lcd_write_cmd(ST7735_CMD_MADCTL);
    uint8_t madctl = LCD_COLOR_ORDER_BGR ? 0x08 : 0x00;
    lcd_write_data(&madctl, 1);

    lcd_write_cmd(ST7735_CMD_DISPON);
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(LCD_BACKLIGHT_PIN, LCD_BACKLIGHT_ON_LEVEL);

    s_lcd_initialized = true;

    return lcd_fill_screen(BLACK);
}

static const char *TAG = "LCD_FPS";
void lcd_fps_test_task(void)
{

    const uint16_t test_colors[] = {BLACK, RED, GREEN, BLUE, WHITE, YELLOW, CYAN, MAGENTA};
    const size_t color_count = sizeof(test_colors) / sizeof(test_colors[0]);
    size_t color_index = 0;

    uint32_t frame_count = 0;
    int64_t window_start_us = esp_timer_get_time();

    while (1)
    {
        lcd_fill_screen(test_colors[color_index]);
        color_index = (color_index + 1) % color_count;
        frame_count++;

        int64_t now_us = esp_timer_get_time();
        int64_t elapsed_us = now_us - window_start_us;
        if (elapsed_us >= 1000000) {
            float fps = (float)frame_count * 1000000.0f / (float)elapsed_us;
            ESP_LOGI(TAG, "LCD full refresh FPS: %.2f", fps);
            frame_count = 0;
            window_start_us = now_us;
        }

        if ((frame_count & 0x0F) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }        
}

