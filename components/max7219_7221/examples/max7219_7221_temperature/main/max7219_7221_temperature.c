// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include <float.h>
#include <string.h>

#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>

#include <esp_log.h>
#include <esp_check.h>
#include <driver/temperature_sensor.h>

#include "max7219_7221.h"

const char* TAG = "max72[19|21]_temperature";

//
// NOTE: For maximum performance, prefer IO MUX over GPIO Matrix routing
//

// SPI Host ID
const spi_host_device_t SPI_HOSTID = SPI2_HOST;

// SPI pins - Depends on the chip and the board
#if CONFIG_IDF_TARGET_ESP32
const gpio_num_t CS_LOAD_PIN = GPIO_NUM_19;
const gpio_num_t CLK_PIN = GPIO_NUM_18;
const gpio_num_t DIN_PIN = GPIO_NUM_16;
#else
#if CONFIG_IDF_TARGET_ESP32S3
const gpio_num_t CS_LOAD_PIN = GPIO_NUM_10;
const gpio_num_t CLK_PIN = GPIO_NUM_12;
const gpio_num_t DIN_PIN = GPIO_NUM_11;
#endif
#endif

// Number of devices MAX7219 / MAX7221 in the chain
const uint8_t ChainLength = 3;

// Time between two display update
const TickType_t DelayBetweenUpdates = pdMS_TO_TICKS(1000);


// Handle to on chip temperature sensor
temperature_sensor_handle_t temperatureSensor_handle = NULL;

// Handle to the MAX7219 / MAX7221 driver
led_driver_max7219_handle_t led_max7219_handle = NULL;


static esp_err_t display_temp_min_max(float currentTemp, float minTemp, float maxTemp);
static void string_to_max7219_symbols(char str[8], uint8_t startDigit, uint8_t symbols[MAX7219_MAX_DIGIT]);


void app_main(void) {
    // Initialize and enable the temperature sensor
    temperature_sensor_config_t tempSensorConfig = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10.0f, 80.0f);
    ESP_ERROR_CHECK(temperature_sensor_install(&tempSensorConfig, &temperatureSensor_handle));
    ESP_ERROR_CHECK(temperature_sensor_enable(temperatureSensor_handle));

    // Configure SPI bus to communicate with MAX7219 / MAX7221
    spi_bus_config_t spiBusConfig = {
        .mosi_io_num = DIN_PIN,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = CLK_PIN,

        .data2_io_num = GPIO_NUM_NC,
        .data3_io_num = GPIO_NUM_NC,

        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOSTID, &spiBusConfig, SPI_DMA_CH_AUTO));

    // Initialize the MAX7219 / MAX7221 driver
    max7219_config_t max7219InitConfig = {
        .spi_cfg = {
            .host_id = SPI_HOSTID,

            .clock_source = SPI_CLK_SRC_DEFAULT,
            .clock_speed_hz = 10 * 1000000,

            .spics_io_num = CS_LOAD_PIN,
            .queue_size = 8
        },
        .hw_config = {
            .chain_length = ChainLength
        }
    };
    ESP_LOGI(TAG, "Initialize MAX7219 / MAX7221 driver");
    ESP_ERROR_CHECK(led_driver_max7219_init(&max7219InitConfig, &led_max7219_handle));
    // NOTE: On power on, the MAX7219 / MAX7221 starts in shutdown mode - All blank, scan mode is 1 digit, no CODE B decode, intensity is minimum

    // Configure scan limit on all devices
    ESP_LOGI(TAG, "Configure scan limit to all digits (8)");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_scan_limit(led_max7219_handle, 8));

    // Configure decode mode to 'direct adressing' for all digits
    ESP_LOGI(TAG, "Configure direct addressing on all digits in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_decode(led_max7219_handle, MAX7219_CODE_B_DECODE_NONE));

    // Set intensity on all devices - MAX7219_INTENSITY_DUTY_CYCLE_STEP_2 is dim
    ESP_LOGI(TAG, "Set intensity to 'MAX7219_INTENSITY_DUTY_CYCLE_STEP_2' on all devices in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_intensity(led_max7219_handle, MAX7219_INTENSITY_DUTY_CYCLE_STEP_2));

    // Reset all digits to 'blank' for a clean visual effect
    ESP_LOGI(TAG, "Set all digits to blank");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain(led_max7219_handle, MAX7219_DIRECT_ADDRESSING_BLANK));

    // Switch to 'normal' mode so digits can be displayed and hold 'all blank' for a little while
    ESP_LOGI(TAG, "Set Normal mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_max7219_handle, MAX7219_NORMAL_MODE));

    // TODO: These are not the right constants for this application
    float minTemp = FLT_MAX;
    float maxTemp = -FLT_MAX;

    do {
        float currentTemp = 0.0f;
        esp_err_t err = temperature_sensor_get_celsius(temperatureSensor_handle, &currentTemp);
        if (err == ESP_OK) {
            if (currentTemp < minTemp) {
                minTemp = currentTemp;
            }
            if (currentTemp > maxTemp) {
                maxTemp = currentTemp;
            }

            ESP_LOGI(TAG, "Temp: %.02f C - Min: %.02f C - Max %.02f C", currentTemp, minTemp, maxTemp);

            err = display_temp_min_max(currentTemp, minTemp, maxTemp);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to update temperature display '%d'", err);
            }
        } else {
            ESP_LOGE(TAG, "Unable to read ESP temperature sensor '%d'", err);
        }

        vTaskDelay(DelayBetweenUpdates);
    } while (true);

    // Shutdown MAX7219 / MAX7221  driver and SPI bus
    ESP_ERROR_CHECK(led_driver_max7219_free(led_max7219_handle));
    ESP_ERROR_CHECK(spi_bus_free(SPI_HOSTID));

    // Disable and uninstall the temperature sensor
    ESP_ERROR_CHECK(temperature_sensor_disable(temperatureSensor_handle));
    ESP_ERROR_CHECK(temperature_sensor_uninstall(temperatureSensor_handle));
}

static esp_err_t display_temp_min_max(float currentTemp, float minTemp, float maxTemp) {
    const uint8_t CurrentTempChainId = 1;
    const uint8_t MinimumTempChainId = 2;
    const uint8_t MaximumTempChainId = 3;

    {
        char temp_str[8];
        sprintf(temp_str, "%.01f", currentTemp);

        uint8_t symbols[MAX7219_MAX_DIGIT] = {
            MAX7219_DIRECT_ADDRESSING_C,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK
        };
        string_to_max7219_symbols(temp_str, 3, symbols);
        ESP_RETURN_ON_ERROR(led_driver_max7219_set_digits(led_max7219_handle, CurrentTempChainId, 1, symbols, MAX7219_MAX_DIGIT), TAG, "Failed to update current temperature");
    }

    {
        char temp_str[8];
        sprintf(temp_str, "%.01f", minTemp);

        uint8_t symbols[MAX7219_MAX_DIGIT] = {
            MAX7219_DIRECT_ADDRESSING_C,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_0,
            MAX7219_DIRECT_ADDRESSING_L
        };
        sprintf(temp_str, "%.01f", minTemp);
        string_to_max7219_symbols(temp_str, 3, symbols);
        ESP_RETURN_ON_ERROR(led_driver_max7219_set_digits(led_max7219_handle, MinimumTempChainId, 1, symbols, MAX7219_MAX_DIGIT), TAG, "Failed to update minimum temperature");
    }

    {
        char temp_str[8];
        sprintf(temp_str, "%.01f", maxTemp);

        uint8_t symbols[MAX7219_MAX_DIGIT] = {
            MAX7219_DIRECT_ADDRESSING_C,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_BLANK,
            MAX7219_DIRECT_ADDRESSING_1,
            MAX7219_DIRECT_ADDRESSING_H
        };
        sprintf(temp_str, "%.01f", maxTemp);
        string_to_max7219_symbols(temp_str, 3, symbols);
        ESP_RETURN_ON_ERROR(led_driver_max7219_set_digits(led_max7219_handle, MaximumTempChainId, 1, symbols, MAX7219_MAX_DIGIT), TAG, "Failed to update maximum temperature");
    }
    
    return ESP_OK;
}

static void string_to_max7219_symbols(char str[8], uint8_t startDigit, uint8_t symbols[MAX7219_MAX_DIGIT]) {
    size_t length = strlen(str);
    uint8_t digitIndex = startDigit - 1;
    bool decimalPoint = false;

    for (int8_t strIndex = length - 1; strIndex >= 0; strIndex--) {
        switch (str[strIndex]) {
            case '.':
                decimalPoint = true;
                continue;
                break;

            case '-':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_MINUS;
                break;

            case ' ':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_BLANK;
                break;

            case '0':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_0;
                break;

            case '1':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_1;
                break;

            case '2':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_2;
                break;

            case '3':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_3;
                break;

            case '4':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_4;
                break;

            case '5':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_5;
                break;
            
            case '6':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_6;
                break;

            case '7':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_7;
                break;

            case '8':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_8;
                break;

            case '9':
                symbols[digitIndex] = MAX7219_DIRECT_ADDRESSING_9;
                break;
        }

        if (decimalPoint == true) {
            symbols[digitIndex] |= MAX7219_SEGMENT_DP;
            decimalPoint = false;
        }

        digitIndex++;
    }
}       