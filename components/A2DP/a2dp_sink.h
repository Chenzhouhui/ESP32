#ifndef A2DP_SINK_H
#define A2DP_SINK_H

#include "esp_err.h"

#define A2DP_SPEAKER_NAME "ESP32_Music_Box"
#define A2DP_DEVICE_NAME A2DP_SPEAKER_NAME

esp_err_t a2dp_sink_start(const char *device_name);
const char *a2dp_sink_get_device_name(void);
const char *a2dp_sink_get_status(void);

#endif
