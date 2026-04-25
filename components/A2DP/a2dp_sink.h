#ifndef A2DP_SINK_H
#define A2DP_SINK_H

#include "esp_err.h"

#define A2DP_DEVICE_NAME "ESP32_Music_Box"

esp_err_t a2dp_sink_start(const char *device_name);

#endif
