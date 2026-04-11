#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "key.h"

void app_main(void)
{
	led_init(GPIO_NUM_2);
	key_init(GPIO_NUM_0);

	while (1) 
    {
        bool key_pressed = key_get_state();
        if (key_pressed) 
        {
            led_toggle();
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        vTaskDelay(pdMS_TO_TICKS(10));

	}
}
