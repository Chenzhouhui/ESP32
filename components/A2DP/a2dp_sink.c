#include "a2dp_sink.h"

#include <string.h>

#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "es8311_app.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

static const char *TAG = "A2DP_SINK";
static bool s_started = false;

static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    if (event == ESP_BT_GAP_AUTH_CMPL_EVT) {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "BT auth success: %s", param->auth_cmpl.device_name);
        } else {
            ESP_LOGW(TAG, "BT auth failed, status=%d", param->auth_cmpl.stat);
        }
    }
}

static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGI(TAG, "A2DP conn state=%d", param->conn_stat.state);
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGI(TAG, "A2DP audio state event");
        break;
    case ESP_A2D_AUDIO_CFG_EVT:
        ESP_LOGI(TAG, "A2DP audio cfg received");
        break;
    default:
        break;
    }
}

static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    static uint32_t packet_count = 0;
    static uint32_t byte_count = 0;

    packet_count++;
    byte_count += len;

    if ((packet_count % 200U) == 0U) {
        ESP_LOGI(TAG, "A2DP stream packets=%lu bytes=%lu", (unsigned long)packet_count, (unsigned long)byte_count);
    }

    if (es8311_audio_is_initialized()) {
        size_t bytes_written = 0;
        esp_err_t ret = es8311_audio_write(data, len, &bytes_written, pdMS_TO_TICKS(20));
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ES8311 write failed: %s", esp_err_to_name(ret));
        }
    }
}

esp_err_t a2dp_sink_start(const char *device_name)
{
    if (s_started) {
        return ESP_OK;
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        return ret;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_bt_gap_register_callback(bt_app_gap_cb);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = esp_a2d_register_callback(&bt_app_a2d_cb);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = esp_a2d_sink_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = es8311_audio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ES8311 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_avrc_ct_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    ret = esp_avrc_tg_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    const char *name_to_use = (device_name != NULL && strlen(device_name) > 0) ? device_name : A2DP_DEVICE_NAME;
    ret = esp_bt_gap_set_device_name(name_to_use);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    if (ret != ESP_OK) {
        return ret;
    }

    s_started = true;
    ESP_LOGI(TAG, "A2DP sink started, device name: %s", name_to_use);

    return ESP_OK;
}
