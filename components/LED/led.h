#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "driver/ledc.h"

#define LED0_GPIO   GPIO_NUM_2
#define LED0_KEY_CONTROL     1 // 普通的led控制，开关，反转
#define LED0_LIGHT_PWM       0 // led呼吸灯


esp_err_t led_init(void);
esp_err_t led_set(bool on);
esp_err_t led_toggle(void);

void ledc_pwm_init(void);
void ledc_pwm_set_duty(uint32_t duty);
void led_light_breathing(void);
#endif // LED_H
