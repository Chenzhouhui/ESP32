#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

#define key0_press  1
#define key0_gpio   GPIO_NUM_0

esp_err_t key_init(void);
uint8_t key_scan(void);