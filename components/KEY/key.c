#include "key.h"
static gpio_num_t s_key_gpio = GPIO_NUM_NC;

esp_err_t key_init(gpio_num_t gpio_num) 
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
    io_conf.intr_type = GPIO_INTR_DISABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) 
    {
        return ret;
    }

    s_key_gpio = gpio_num;

    return ESP_OK;
}

bool key_get_state(void) 
{
    if (s_key_gpio == GPIO_NUM_NC) 
    {
        return false; // Key not initialized, return default state
    }
    return gpio_get_level(s_key_gpio) == 0; // Assuming active-low trigger
}