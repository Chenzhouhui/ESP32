#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdlib.h>
#include "led.h"
#include "lcd.h"
#include "GUI.h"

#define FRAME_PIXELS (LCD_W * LCD_H)
#define FRAME_SIZE_RGB332 (FRAME_PIXELS)
#define FRAME_SIZE_RGB565 (FRAME_PIXELS * 2)
#define FRAME_HEADER_SIZE 4
#define UART_RX_BUF_SIZE (FRAME_SIZE_RGB332 * 2)
#define UART_BAUD_RATE 3000000
static const char *TAG = "VIDEO";

static bool receive_one_frame(uint8_t *frame_buffer)
{
    static const uint8_t marker[FRAME_HEADER_SIZE] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t scan_buf[512];
    uint8_t state = 0;
    size_t payload_index = 0;

    while (1) {
        int got = uart_read_bytes(UART_NUM_0, scan_buf, sizeof(scan_buf), pdMS_TO_TICKS(80));
        if (got <= 0) {
            return false;
        }

        for (int index = 0; index < got; ++index) {
            uint8_t byte = scan_buf[index];

            if (state < FRAME_HEADER_SIZE) {
                if (byte == marker[state]) {
                    state++;
                    if (state == FRAME_HEADER_SIZE) {
                        payload_index = 0;
                    }
                } else {
                    state = (byte == marker[0]) ? 1 : 0;
                }
                continue;
            }

            frame_buffer[payload_index++] = byte;
            if (payload_index >= FRAME_SIZE_RGB332) {
                return true;
            }
        }
    }
}

static inline void rgb332_to_rgb565_be(const uint8_t *src, uint8_t *dst)
{
    for (size_t index = 0; index < FRAME_PIXELS; ++index) {
        uint8_t pixel = src[index];
        uint8_t r3 = (uint8_t)(pixel >> 5);
        uint8_t g3 = (uint8_t)((pixel >> 2) & 0x07);
        uint8_t b2 = (uint8_t)(pixel & 0x03);

        uint16_t r5 = (uint16_t)((r3 << 2) | (r3 >> 1));
        uint16_t g6 = (uint16_t)((g3 << 3) | g3);
        uint16_t b5 = (uint16_t)((b2 << 3) | (b2 << 1) | (b2 >> 1));

        uint16_t rgb565 = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
        dst[index * 2] = (uint8_t)(rgb565 >> 8);
        dst[index * 2 + 1] = (uint8_t)(rgb565 & 0xFF);
    }
}

void video_task(void *pvParameters) {
    uint8_t *frame_rgb332 = malloc(FRAME_SIZE_RGB332);
    uint8_t *frame_rgb565 = malloc(FRAME_SIZE_RGB565);
    if (frame_rgb332 == NULL || frame_rgb565 == NULL) {
        ESP_LOGE(TAG, "malloc frame buffer failed");
        free(frame_rgb332);
        free(frame_rgb565);
        vTaskDelete(NULL);
    }

    // 配置串口：2M 波特率，大缓冲区
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    esp_err_t ret = uart_param_config(UART_NUM_0, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        free(frame_rgb332);
        free(frame_rgb565);
        vTaskDelete(NULL);
    }

    uart_driver_delete(UART_NUM_0);

    ret = uart_driver_install(UART_NUM_0, UART_RX_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        free(frame_rgb332);
        free(frame_rgb565);
        vTaskDelete(NULL);
    }

    uart_set_rx_timeout(UART_NUM_0, 2);

    while (1) {
        if (!receive_one_frame(frame_rgb332)) {
            continue;
        }

        rgb332_to_rgb565_be(frame_rgb332, frame_rgb565);
        LCD_DrawBmp16Ex(0, 0, LCD_W, LCD_H, (const unsigned char *)frame_rgb565);
    }
}

void app_main(void)
{
    lcd_init();
    led_init();
    led_set(true);

    BaseType_t task_ok = xTaskCreatePinnedToCore(video_task, "video_task", 8192, NULL, 10, NULL, 1);
    if (task_ok != pdPASS) {
        ESP_LOGE(TAG, "create video_task failed");
    }
        
}
