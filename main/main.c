#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdlib.h>
#include "led.h"
#include "lcd.h"
#include "GUI.h"

#define FRAME_SIZE (128 * 128 * 2) // 128x128 RGB565 的字节数
static const char *TAG = "VIDEO";

static bool wait_frame_header(void)
{
    static const uint8_t marker[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t state = 0;
    uint8_t byte = 0;

    while (1) {
        int got = uart_read_bytes(UART_NUM_0, &byte, 1, pdMS_TO_TICKS(200));
        if (got <= 0) {
            return false;
        }

        if (byte == marker[state]) {
            state++;
            if (state == sizeof(marker)) {
                return true;
            }
        } else {
            state = (byte == marker[0]) ? 1 : 0;
        }
    }
}

static bool read_frame_payload(uint8_t *frame_buffer)
{
    size_t read_len = 0;
    while (read_len < FRAME_SIZE) {
        int got = uart_read_bytes(UART_NUM_0,
                                  frame_buffer + read_len,
                                  FRAME_SIZE - read_len,
                                  pdMS_TO_TICKS(80));
        if (got <= 0) {
            return false;
        }
        read_len += (size_t)got;
    }
    return true;
}

void video_task(void *pvParameters) {
    uint8_t *frame_buffer = malloc(FRAME_SIZE);
    if (frame_buffer == NULL) {
        ESP_LOGE(TAG, "malloc frame buffer failed");
        vTaskDelete(NULL);
    }

    // 配置串口：2M 波特率，大缓冲区
    uart_config_t uart_config = {
        .baud_rate = 2000000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    esp_err_t ret = uart_param_config(UART_NUM_0, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        free(frame_buffer);
        vTaskDelete(NULL);
    }

    ret = uart_driver_install(UART_NUM_0, FRAME_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        free(frame_buffer);
        vTaskDelete(NULL);
    }

    while (1) {
        if (!wait_frame_header()) {
            continue;
        }

        if (!read_frame_payload(frame_buffer)) {
            continue;
        }

        LCD_DrawBmp16Ex(0, 0, 128, 128, (const unsigned char *)frame_buffer);
    }
}

void app_main(void)
{
    lcd_init();
    led_init();
    led_set(true);

    BaseType_t task_ok = xTaskCreatePinnedToCore(video_task, "video_task", 8192, NULL, 5, NULL, 1);
    if (task_ok != pdPASS) {
        ESP_LOGE(TAG, "create video_task failed");
    }
        
}
