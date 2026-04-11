#include "led.h"

static bool s_initialized = false;
static bool s_led_on = false;

esp_err_t led_init(void)
{
	if (!GPIO_IS_VALID_OUTPUT_GPIO(led0_gpio)) 
    {
		return ESP_ERR_INVALID_ARG;
	}

	gpio_config_t io_conf; 
	io_conf.pin_bit_mask = 1ULL << led0_gpio;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	

	esp_err_t ret = gpio_config(&io_conf);
	if (ret != ESP_OK) 
    {
		return ret;
	}

	s_initialized = true;
    s_led_on = false;

	return gpio_set_level(led0_gpio, 0);
}

esp_err_t led_set(bool on)
{
	if (!s_initialized) 
    {
		return ESP_ERR_INVALID_STATE;
	}

    s_led_on = on;

	return gpio_set_level(led0_gpio, on ? 1 : 0);
}
esp_err_t led_toggle(void)
{
	if (!s_initialized) 
	{
		return ESP_ERR_INVALID_STATE;
	}

    s_led_on = !s_led_on;
	return gpio_set_level(led0_gpio, s_led_on ? 1 : 0);
}