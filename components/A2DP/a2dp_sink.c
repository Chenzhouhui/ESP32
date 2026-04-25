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
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "A2DP_SINK";
static bool s_started = false;
static const char *s_device_name = A2DP_DEVICE_NAME;
static const char *s_status = "Idle";
static StreamBufferHandle_t s_audio_stream = NULL;
static TaskHandle_t s_audio_tx_task_handle = NULL;

#define A2DP_AUDIO_STREAM_SIZE_BYTES   (48 * 1024)
#define A2DP_AUDIO_TRIGGER_LEVEL_BYTES (512)
#define A2DP_AUDIO_TX_CHUNK_BYTES      (1024)

static uint32_t a2dp_sbc_get_sample_rate_hz(const esp_a2d_mcc_t *mcc)
{
    if (mcc == NULL || mcc->type != ESP_A2D_MCT_SBC) {
        return 44100;
    }

    uint8_t oct0 = mcc->cie.sbc[0];
    if (oct0 & 0x40) {
        return 32000;
    }
    if (oct0 & 0x20) {
        return 44100;
    }
    if (oct0 & 0x10) {
        return 48000;
    }
    if (oct0 & 0x80) {
        return 16000;
    }
    return 44100;
}

static void a2dp_audio_tx_task(void *arg)
{
    uint8_t tx_buf[A2DP_AUDIO_TX_CHUNK_BYTES];
    uint32_t write_timeout_count = 0;

    while (1) {
        size_t received = xStreamBufferReceive(s_audio_stream,
                                               tx_buf,
                                               sizeof(tx_buf),
                                               pdMS_TO_TICKS(200));
        if (received == 0) {
            continue;
        }

        size_t offset = 0;
        while (offset < received) {
            size_t bytes_written = 0;
            esp_err_t ret = es8311_audio_write(tx_buf + offset,
                                               received - offset,
                                               &bytes_written,
                                               portMAX_DELAY);
            if (ret == ESP_OK && bytes_written > 0) {
                offset += bytes_written;
                continue;
            }

            if (ret == ESP_ERR_TIMEOUT) {
                write_timeout_count++;
                if ((write_timeout_count % 200U) == 0U) {
                    ESP_LOGW(TAG, "ES8311 write timeout x%lu", (unsigned long)write_timeout_count);
                }
                break;
            }

            ESP_LOGW(TAG, "ES8311 write failed: %s", esp_err_to_name(ret));
            break;
        }
    }
}

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
        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            s_status = "Connected";
        } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING) {
            s_status = "Connecting";
        } else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTING) {
            s_status = "Disconnecting";
        } else {
            s_status = "Discoverable";
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGI(TAG, "A2DP audio state event");
        if (param->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
            s_status = "Playing";
        } else {
            s_status = "Connected";
        }
        break;
    case ESP_A2D_AUDIO_CFG_EVT:
    {
        uint32_t sample_rate_hz = a2dp_sbc_get_sample_rate_hz(&param->audio_cfg.mcc);
        ESP_LOGI(TAG, "A2DP audio cfg received, sample_rate=%lu", (unsigned long)sample_rate_hz);
        esp_err_t ret = es8311_audio_set_sample_rate(sample_rate_hz);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ES8311 set sample rate failed: %s", esp_err_to_name(ret));
        }
        break;
    }
    default:
        break;
    }
}

static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    static uint32_t packet_count = 0;
    static uint32_t byte_count = 0;
    static uint32_t drop_bytes = 0;

    packet_count++;
    byte_count += len;

    if ((packet_count % 200U) == 0U) {
        ESP_LOGI(TAG, "A2DP stream packets=%lu bytes=%lu", (unsigned long)packet_count, (unsigned long)byte_count);
    }

    if (s_audio_stream != NULL) {
        size_t sent = xStreamBufferSend(s_audio_stream, data, len, 0);
        if (sent < len) {
            drop_bytes += (uint32_t)(len - sent);
            if ((packet_count % 200U) == 0U) {
                ESP_LOGW(TAG, "A2DP buffer overflow, dropped=%lu bytes", (unsigned long)drop_bytes);
            }
        }
    }
}

esp_err_t a2dp_sink_start(const char *device_name)
{
    if (s_started) {
        return ESP_OK;
    }

    s_status = "Starting";

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        s_status = "NVS Error";
        return ret;
    }

    ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "BT BLE mem release failed: %s", esp_err_to_name(ret));
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        s_status = "BT Error";
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        s_status = "BT Error";
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        s_status = "BT Error";
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        s_status = "BT Error";
        return ret;
    }

    ret = esp_bt_gap_register_callback(bt_app_gap_cb);
    if (ret != ESP_OK) {
        s_status = "BT Error";
        return ret;
    }

    ret = esp_avrc_ct_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        s_status = "AVRCP Error";
        return ret;
    }

    ret = esp_avrc_tg_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        s_status = "AVRCP Error";
        return ret;
    }

    ret = esp_a2d_register_callback(&bt_app_a2d_cb);
    if (ret != ESP_OK) {
        s_status = "A2DP Error";
        return ret;
    }

    ret = esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
    if (ret != ESP_OK) {
        s_status = "A2DP Error";
        return ret;
    }

    ret = esp_a2d_sink_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = es8311_audio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ES8311 init failed: %s", esp_err_to_name(ret));
        s_status = "Codec Error";
        return ret;
    }

    s_audio_stream = xStreamBufferCreate(A2DP_AUDIO_STREAM_SIZE_BYTES, A2DP_AUDIO_TRIGGER_LEVEL_BYTES);
    if (s_audio_stream == NULL) {
        ESP_LOGE(TAG, "A2DP stream buffer alloc failed (%d bytes)", A2DP_AUDIO_STREAM_SIZE_BYTES);
        s_status = "No Memory";
        return ESP_ERR_NO_MEM;
    }

    BaseType_t task_ok = xTaskCreate(a2dp_audio_tx_task,
                                     "a2dp_audio_tx",
                                     3072,
                                     NULL,
                                     configMAX_PRIORITIES - 1,
                                     &s_audio_tx_task_handle);
    if (task_ok != pdPASS) {
        ESP_LOGE(TAG, "A2DP TX task create failed");
        vStreamBufferDelete(s_audio_stream);
        s_audio_stream = NULL;
        s_status = "No Memory";
        return ESP_ERR_NO_MEM;
    }

    const char *name_to_use = (device_name != NULL && strlen(device_name) > 0) ? device_name : A2DP_DEVICE_NAME;
    s_device_name = name_to_use;
    ret = esp_bt_gap_set_device_name(name_to_use);
    if (ret != ESP_OK) {
        s_status = "Name Error";
        return ret;
    }

    ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    if (ret != ESP_OK) {
        s_status = "Scan Error";
        return ret;
    }

    s_started = true;
    s_status = "Discoverable";
    ESP_LOGI(TAG, "A2DP sink started, device name: %s", name_to_use);

    return ESP_OK;
}

const char *a2dp_sink_get_device_name(void)
{
    return s_device_name;
}

const char *a2dp_sink_get_status(void)
{
    return s_status;
}
