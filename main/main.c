#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "key.h"

void app_main(void)
{

    led_init();
    key_init();

    while (1)
    {
        if (key_scan() == key0_press)
        {
            led_toggle();
            printf("Key pressed!\n");
            printf("LED state toggled!\n");
        }
        
        vTaskDelay(10);
    }
}
