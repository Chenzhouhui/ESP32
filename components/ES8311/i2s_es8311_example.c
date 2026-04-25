/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_check.h"
#include "es8311.h"
#include "es8311_app.h"
#include "example_config.h"

static const char *TAG = "i2s_es8311";
static i2s_chan_handle_t tx_handle = NULL;
static es8311_handle_t s_es8311_handle = NULL;
static bool s_audio_initialized = false;
static uint32_t s_sample_rate_hz = EXAMPLE_SAMPLE_RATE;

void es8311_task(void *args)
{
    printf("i2s es8311 codec example start\n-----------------------------\n");

    if (es8311_audio_init() != ESP_OK) {
        ESP_LOGE(TAG, "es8311 audio init failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "es8311 audio ready for external stream");

    vTaskDelete(NULL);
}

esp_err_t es8311_audio_init(void)
{
    if (s_audio_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(i2s_driver_init(), TAG, "i2s driver init failed");
    ESP_LOGI(TAG, "i2s driver init success");

    ESP_RETURN_ON_ERROR(es8311_codec_init(), TAG, "es8311 codec init failed");
    ESP_LOGI(TAG, "es8311 codec init success");

#if EXAMPLE_PA_CTRL_IO >= 0
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << EXAMPLE_PA_CTRL_IO),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_cfg), TAG, "PA pin config failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(EXAMPLE_PA_CTRL_IO, 1), TAG, "PA enable failed");
#endif

    s_audio_initialized = true;
    return ESP_OK;
}

esp_err_t es8311_audio_write(const uint8_t *data, size_t len, size_t *bytes_written, TickType_t ticks_to_wait)
{
    if (!s_audio_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t written = 0;
    esp_err_t ret = i2s_channel_write(tx_handle, data, len, &written, ticks_to_wait);
    if (bytes_written != NULL) {
        *bytes_written = written;
    }
    return ret;
}

esp_err_t es8311_audio_set_sample_rate(uint32_t sample_rate_hz)
{
    if (!s_audio_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (sample_rate_hz < 8000 || sample_rate_hz > 96000) {
        return ESP_ERR_INVALID_ARG;
    }
    if (sample_rate_hz == s_sample_rate_hz) {
        return ESP_OK;
    }

    esp_err_t ret = i2s_channel_disable(tx_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate_hz);
    clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;
    ret = i2s_channel_reconfig_std_clock(tx_handle, &clk_cfg);
    if (ret != ESP_OK) {
        i2s_channel_enable(tx_handle);
        return ret;
    }

    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    const int codec_mclk_hz = (int)(sample_rate_hz * 32U);
    ret = es8311_sample_frequency_config(s_es8311_handle, codec_mclk_hz, (int)sample_rate_hz);
    if (ret != ESP_OK) {
        return ret;
    }

    s_sample_rate_hz = sample_rate_hz;
    ESP_LOGI(TAG, "sample rate updated to %lu Hz", (unsigned long)sample_rate_hz);
    return ESP_OK;
}

bool es8311_audio_is_initialized(void)
{
    return s_audio_initialized;
}

static esp_err_t es8311_codec_init(void)
{
    /* Initialize I2C peripheral */
#if !defined(CONFIG_EXAMPLE_BSP)
    const i2c_config_t es_i2c_cfg = {
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "config i2c failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER,  0, 0, 0), TAG, "install i2c driver failed");
#else
    ESP_ERROR_CHECK(bsp_i2c_init());
#endif

    /* Initialize es8311 codec */
    s_es8311_handle = es8311_create(I2C_NUM, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(s_es8311_handle, ESP_FAIL, TAG, "es8311 create failed");
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = false,
        .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
        .sample_frequency = EXAMPLE_SAMPLE_RATE
    };

    ESP_RETURN_ON_ERROR(es8311_init(s_es8311_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16), TAG, "init es8311 failed");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(s_es8311_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(s_es8311_handle, false), TAG, "set es8311 microphone failed");
#if CONFIG_EXAMPLE_MODE_ECHO
    ESP_RETURN_ON_ERROR(es8311_microphone_gain_set(s_es8311_handle, EXAMPLE_MIC_GAIN), TAG, "set es8311 microphone gain failed");
#endif
    return ESP_OK;
}

static esp_err_t i2s_driver_init(void)
{
#if !defined(CONFIG_EXAMPLE_BSP)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &tx_handle, NULL), TAG, "create i2s channel failed");
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = GPIO_NUM_NC,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;

    esp_err_t ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init tx channel failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_handle), TAG, "enable tx channel failed");
#else
    ESP_LOGI(TAG, "Using BSP for HW configuration");
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;
    ESP_ERROR_CHECK(bsp_audio_init(&std_cfg, &tx_handle, NULL));
    ESP_ERROR_CHECK(bsp_audio_poweramp_enable(true));
#endif
    return ESP_OK;
}



