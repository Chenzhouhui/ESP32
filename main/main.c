#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"
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
    LCD_ShowString(10, 10, 16, (uint8_t *)"Hello", 0);

    esp_err_t ret = a2dp_sink_start(A2DP_DEVICE_NAME);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "A2DP start failed: %s", esp_err_to_name(ret));
    }

    while (1)
    {
        
        vTaskDelay(1);
    }
        


}
