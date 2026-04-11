#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t led_init(gpio_num_t gpio_num);
esp_err_t led_set(bool on);
void led_toggle(void);
