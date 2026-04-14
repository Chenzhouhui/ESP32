#include "led.h"

#if LED0_KEY_CONTROL == 1
static bool s_initialized = false;
static bool s_led_on = false;

esp_err_t led_init(void)
{
	if (!GPIO_IS_VALID_OUTPUT_GPIO(LED0_GPIO)) 
    {
		return ESP_ERR_INVALID_ARG;
	}

	gpio_config_t io_conf = {0}; 
	io_conf.pin_bit_mask = 1ULL << LED0_GPIO;
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

	return gpio_set_level(LED0_GPIO, 0);
}

esp_err_t led_set(bool on)
{
	if (!s_initialized) 
    {
		return ESP_ERR_INVALID_STATE;
	}

    s_led_on = on;

	return gpio_set_level(LED0_GPIO, on ? 1 : 0);
}
esp_err_t led_toggle(void)
{
	if (!s_initialized) 
	{
		return ESP_ERR_INVALID_STATE;
	}

    s_led_on = !s_led_on;
	return gpio_set_level(LED0_GPIO, s_led_on ? 1 : 0);
}
#elif LED0_LIGHT_PWM == 1
void ledc_pwm_init(void)
{
	ledc_timer_config_t ledc_timer_pwm = {
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.timer_num = LEDC_TIMER_0,
		.duty_resolution = LEDC_TIMER_8_BIT,
		.freq_hz = 5000,
		.clk_cfg = LEDC_AUTO_CLK
	};
	ledc_timer_config(&ledc_timer_pwm);
	ledc_channel_config_t ledc_channel_pwm = {
		.speed_mode = LEDC_HIGH_SPEED_MODE,
		.channel = LEDC_CHANNEL_0,
		.timer_sel = LEDC_TIMER_0,
		.intr_type = LEDC_INTR_DISABLE,
		.gpio_num = LED0_GPIO,
		.duty = 0,
		.hpoint = 0
	};	
	ledc_channel_config(&ledc_channel_pwm);
	ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}
void ledc_pwm_set_duty(uint32_t duty)
{
	ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}
void led_light_breathing(void)
{
	for (int duty = 0; duty <= 255; duty++) {
		ledc_pwm_set_duty(duty);
		vTaskDelay(10);
	}
	for (int duty = 255; duty >= 0; duty--) {
		ledc_pwm_set_duty(duty);
		vTaskDelay(10);
	}
}
#endif // LED0_KEY_CONTROL