#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "led.h"
#include "lcd.h"
#include "GUI.h"

#define FRAME_PIXELS (LCD_W * LCD_H)
#define FRAME_SIZE_RGB332 (FRAME_PIXELS)
#define FRAME_SIZE_RGB565 (FRAME_PIXELS * 2)
#define FRAME_HEADER_SIZE 4
#define UART_RX_BUF_SIZE (FRAME_SIZE_RGB332 * 2)
#define UART_BAUD_RATE 3000000

#define FRAME_TYPE_FULL  0x01
#define FRAME_TYPE_DELTA 0x02

#define BLOCK_SIZE 8
#define BLOCK_PIXELS (BLOCK_SIZE * BLOCK_SIZE)
#define BLOCKS_X (LCD_W / BLOCK_SIZE)
#define BLOCKS_Y (LCD_H / BLOCK_SIZE)
#define BLOCK_COUNT (BLOCKS_X * BLOCKS_Y)
#define DELTA_MASK_BYTES (BLOCK_COUNT / 8)
#define MAX_PAYLOAD_SIZE (FRAME_SIZE_RGB332 + DELTA_MASK_BYTES)

static const char *TAG = "VIDEO";
static const uint8_t s_frame_marker[FRAME_HEADER_SIZE] = {0xAA, 0xBB, 0xCC, 0xDD};
static uint8_t s_lut_hi[256];
static uint8_t s_lut_lo[256];

static void init_rgb332_lut(void)
{
    for (int value = 0; value < 256; ++value) {
        uint8_t pixel = (uint8_t)value;
        uint8_t r3 = (uint8_t)(pixel >> 5);
        uint8_t g3 = (uint8_t)((pixel >> 2) & 0x07);
        uint8_t b2 = (uint8_t)(pixel & 0x03);

        uint16_t r5 = (uint16_t)((r3 << 2) | (r3 >> 1));
        uint16_t g6 = (uint16_t)((g3 << 3) | g3);
        uint16_t b5 = (uint16_t)((b2 << 3) | (b2 << 1) | (b2 >> 1));
        uint16_t rgb565 = (uint16_t)((r5 << 11) | (g6 << 5) | b5);

        s_lut_hi[value] = (uint8_t)(rgb565 >> 8);
        s_lut_lo[value] = (uint8_t)(rgb565 & 0xFF);
    }
}

static bool uart_read_exact(uint8_t *buffer, size_t len, TickType_t timeout)
{
    size_t got_total = 0;
    while (got_total < len) {
        int got = uart_read_bytes(UART_NUM_0, buffer + got_total, len - got_total, timeout);
        if (got <= 0) {
            return false;
        }
        got_total += (size_t)got;
    }
    return true;
}

static bool sync_frame_header(void)
{
    uint8_t header[FRAME_HEADER_SIZE];
    if (!uart_read_exact(header, FRAME_HEADER_SIZE, pdMS_TO_TICKS(200))) {
        return false;
    }

    while (1) {
        if (memcmp(header, s_frame_marker, FRAME_HEADER_SIZE) == 0) {
            return true;
        }

        memmove(header, header + 1, FRAME_HEADER_SIZE - 1);
        if (!uart_read_exact(&header[FRAME_HEADER_SIZE - 1], 1, pdMS_TO_TICKS(80))) {
            return false;
        }
    }
}

static bool receive_packet(uint8_t *frame_type, uint16_t *payload_len, uint8_t *payload)
{
    if (!sync_frame_header()) {
        return false;
    }

    uint8_t meta[3];
    if (!uart_read_exact(meta, sizeof(meta), pdMS_TO_TICKS(80))) {
        return false;
    }

    *frame_type = meta[0];
    *payload_len = (uint16_t)meta[1] | ((uint16_t)meta[2] << 8);

    if (*payload_len > MAX_PAYLOAD_SIZE) {
        return false;
    }

    if (*payload_len == 0) {
        return true;
    }

    return uart_read_exact(payload, *payload_len, pdMS_TO_TICKS(120));
}

static bool apply_delta_frame(uint8_t *frame_rgb332, const uint8_t *payload, uint16_t payload_len)
{
    if (payload_len < DELTA_MASK_BYTES) {
        return false;
    }

    const uint8_t *mask = payload;
    const uint8_t *block_data = payload + DELTA_MASK_BYTES;
    size_t block_data_len = (size_t)payload_len - DELTA_MASK_BYTES;
    if ((block_data_len % BLOCK_PIXELS) != 0) {
        return false;
    }

    size_t offset = 0;
    for (int block = 0; block < BLOCK_COUNT; ++block) {
        uint8_t mask_byte = mask[block >> 3];
        uint8_t mask_bit = (uint8_t)(1U << (block & 0x07));
        if ((mask_byte & mask_bit) == 0) {
            continue;
        }

        if (offset + BLOCK_PIXELS > block_data_len) {
            return false;
        }

        int block_x = block % BLOCKS_X;
        int block_y = block / BLOCKS_X;
        for (int row = 0; row < BLOCK_SIZE; ++row) {
            size_t dst_index = (size_t)(block_y * BLOCK_SIZE + row) * LCD_W + (size_t)block_x * BLOCK_SIZE;
            memcpy(frame_rgb332 + dst_index, block_data + offset + (size_t)row * BLOCK_SIZE, BLOCK_SIZE);
        }

        offset += BLOCK_PIXELS;
    }

    return offset == block_data_len;
}

static inline void rgb332_to_rgb565_be(const uint8_t *src, uint8_t *dst)
{
    for (size_t index = 0; index < FRAME_PIXELS; ++index) {
        uint8_t pixel = src[index];
        dst[index * 2] = s_lut_hi[pixel];
        dst[index * 2 + 1] = s_lut_lo[pixel];
    }
}

static bool decode_packet_to_frame(uint8_t frame_type,
                                   const uint8_t *payload,
                                   uint16_t payload_len,
                                   uint8_t *frame_rgb332)
{
    if (frame_type == FRAME_TYPE_FULL) {
        if (payload_len != FRAME_SIZE_RGB332) {
            return false;
        }
        memcpy(frame_rgb332, payload, FRAME_SIZE_RGB332);
        return true;
    }

    if (frame_type == FRAME_TYPE_DELTA) {
        return apply_delta_frame(frame_rgb332, payload, payload_len);
    }

    return false;
}

void video_task(void *pvParameters) {
    uint8_t *frame_rgb332 = malloc(FRAME_SIZE_RGB332);
    uint8_t *frame_rgb565 = malloc(FRAME_SIZE_RGB565);
    uint8_t *packet_payload = malloc(MAX_PAYLOAD_SIZE);
    if (frame_rgb332 == NULL || frame_rgb565 == NULL || packet_payload == NULL) {
        ESP_LOGE(TAG, "malloc frame buffer failed");
        free(frame_rgb332);
        free(frame_rgb565);
        free(packet_payload);
        vTaskDelete(NULL);
    }

    memset(frame_rgb332, 0, FRAME_SIZE_RGB332);
    init_rgb332_lut();

    // 配置串口：4M 波特率，大缓冲区
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
        free(packet_payload);
        vTaskDelete(NULL);
    }

    uart_driver_delete(UART_NUM_0);

    ret = uart_driver_install(UART_NUM_0, UART_RX_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        free(frame_rgb332);
        free(frame_rgb565);
        free(packet_payload);
        vTaskDelete(NULL);
    }

    uart_set_rx_timeout(UART_NUM_0, 2);

    while (1) {
        uint8_t frame_type = 0;
        uint16_t payload_len = 0;

        if (!receive_packet(&frame_type, &payload_len, packet_payload)) {
            continue;
        }

        if (!decode_packet_to_frame(frame_type, packet_payload, payload_len, frame_rgb332)) {
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
