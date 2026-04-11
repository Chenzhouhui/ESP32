#include "key.h"
#include "freertos/FreeRTOS.h"
static gpio_num_t s_key_gpio = GPIO_NUM_NC;
static bool s_initialized = false;
static int s_key_state = 0;
static volatile TickType_t s_last_isr_tick = 0;

static IRAM_ATTR void key_isr_handler(void* arg)
{
    (void)arg;
    TickType_t now = xTaskGetTickCountFromISR();
    if ((now - s_last_isr_tick) >= pdMS_TO_TICKS(30)) 
    {
        s_last_isr_tick = now;
        if(gpio_get_level(s_key_gpio) == 0) 
        {
             s_key_state = 1;
        }
    }

}
esp_err_t key_init(void) 
{
    if (!GPIO_IS_VALID_GPIO(key0_gpio)) 
    {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_conf;
    io_conf.pin_bit_mask = 1ULL << key0_gpio;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) 
    {
        return ret;
    }

    s_key_gpio = key0_gpio;
    s_initialized = true;
    gpio_install_isr_service(0); // Install GPIO ISR service with default configuration
    gpio_isr_handler_add(key0_gpio, key_isr_handler, (void*)key0_gpio); // Add dummy ISR handler for the key GPIO to enable interrupt functionality
    gpio_intr_enable(key0_gpio); // Enable GPIO interrupt for the key button
    return ESP_OK;
}

uint8_t key_scan(void) 
{
    if(s_key_state == 1) 
    {
        s_key_state = 0;
        return key0_press;
    }
    return 0;
}