#include "led.h"

static gpio_num_t s_led_gpio = GPIO_NUM_NC;
static bool s_initialized = false;

esp_err_t led_init(gpio_num_t gpio_num)
{
	if (!GPIO_IS_VALID_OUTPUT_GPIO(gpio_num)) 
    {
		return ESP_ERR_INVALID_ARG;
	}

	gpio_config_t io_conf; 
	io_conf.pin_bit_mask = 1ULL << gpio_num;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	

	esp_err_t ret = gpio_config(&io_conf);
	if (ret != ESP_OK) 
    {
		return ret;
	}

	s_led_gpio = gpio_num;
	s_initialized = true;

	return gpio_set_level(s_led_gpio, 0);
}

esp_err_t led_set(bool on)
{
	if (!s_initialized) 
    {
		return ESP_ERR_INVALID_STATE;
	}

	return gpio_set_level(s_led_gpio, on ? 1 : 0);
}
void led_toggle(void)
{
    int current_level = gpio_get_level(s_led_gpio);
    gpio_set_level(s_led_gpio, !current_level);
}