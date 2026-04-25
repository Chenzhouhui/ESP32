#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>
#include "led.h"
#include "lcd.h"
#include "GUI.h"
#include "a2dp_sink.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    lcd_init();
    led_init();
    led_set(true);
    lcd_fill_screen(BLACK);

    LCD_ShowString(0, 0, 16, (uint8_t *)"BT Speaker:", 0);
    LCD_ShowString(0, 48, 16, (uint8_t *)"BT Status:", 0);

    esp_err_t ret = a2dp_sink_start(A2DP_DEVICE_NAME);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "A2DP start failed: %s", esp_err_to_name(ret));
    }

    char last_name[32] = {0};
    char last_status[32] = {0};

    while (1)
    {
        const char *name = a2dp_sink_get_device_name();
        const char *status = a2dp_sink_get_status();

        if (name == NULL) {
            name = "Unknown";
        }
        if (status == NULL) {
            status = "Unknown";
        }

        if (strcmp(last_name, name) != 0) {
            lcd_fill_rect(0, 16, LCD_W - 1, 40, BLACK);
            LCD_ShowString(0, 20, 16, (uint8_t *)name, 0);
            strncpy(last_name, name, sizeof(last_name) - 1);
            last_name[sizeof(last_name) - 1] = '\0';
        }

        if (strcmp(last_status, status) != 0) {
            lcd_fill_rect(0, 64, LCD_W - 1, 88, BLACK);
            LCD_ShowString(0, 68, 16, (uint8_t *)status, 0);
            strncpy(last_status, status, sizeof(last_status) - 1);
            last_status[sizeof(last_status) - 1] = '\0';
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
        


}
