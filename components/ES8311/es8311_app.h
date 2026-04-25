#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

void es8311_task(void *args);
esp_err_t es8311_audio_init(void);
esp_err_t es8311_audio_set_sample_rate(uint32_t sample_rate_hz);
esp_err_t es8311_audio_write(const uint8_t *data, size_t len, size_t *bytes_written, TickType_t ticks_to_wait);
bool es8311_audio_is_initialized(void);

#ifdef __cplusplus
}
#endif
