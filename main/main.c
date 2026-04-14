#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "led.h"
#include "lcd.h"
#include "GUI.h"

static const char *TAG = "LCD_FPS";

void app_main(void)
{
    lcd_init();
    led_init();
    led_set(true);
/*
    const u16 test_colors[] = {BLACK, RED, GREEN, BLUE, WHITE, YELLOW, CYAN, MAGENTA};
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
        */


}
