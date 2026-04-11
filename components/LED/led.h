#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

#define led0_gpio   GPIO_NUM_2

esp_err_t led_init(void);
esp_err_t led_set(bool on);
esp_err_t led_toggle(void);
