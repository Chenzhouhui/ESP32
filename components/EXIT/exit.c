#include "exit.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"

static volatile bool s_pressed_event = false;
static volatile TickType_t s_last_isr_tick = 0;

static void IRAM_ATTR key_isr_handler(void* arg)
{
    (void)arg;

    TickType_t now = xTaskGetTickCountFromISR();
    if ((now - s_last_isr_tick) >= pdMS_TO_TICKS(30))
    {
        s_pressed_event = true;
        s_last_isr_tick = now;
    }
}

esp_err_t exit_init(gpio_num_t gpio_num)
{
    if (!GPIO_IS_VALID_GPIO(gpio_num)) 
    {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_conf;
    io_conf.pin_bit_mask = 1ULL << gpio_num;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE; // Trigger on falling edge (button press)

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) 
    {
        return ret;
    }
    ret = gpio_install_isr_service(0); // Install GPIO ISR service with default configuration
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        return ret;
    }
    ret = gpio_isr_handler_add(gpio_num, key_isr_handler, NULL); // Add ISR handler for the exit button GPIO
    if (ret != ESP_OK)
    {
        return ret;
    }

    gpio_intr_enable(gpio_num); // Enable GPIO interrupt for the exit button
    return ESP_OK;
}

bool exit_take_pressed_event(void)
{
    if (s_pressed_event)
    {
        s_pressed_event = false;
        return true;
    }

    return false;
}
