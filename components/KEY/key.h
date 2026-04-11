#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"
esp_err_t key_init(gpio_num_t gpio_num);
bool key_get_state(void);