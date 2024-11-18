// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <esp_check.h>

#include "maxim_7219_7221.h"

const char* TAG = "max72[19|21]_main";

//
// NOTE: For maximum performance, prefer IO MUX over GPIO Matrix routing
//  * See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html#gpio-matrix-routing
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
#else
#if CONFIG_IDF_TARGET_ESP32C3
const gpio_num_t CS_LOAD_PIN = GPIO_NUM_1;
const gpio_num_t CLK_PIN = GPIO_NUM_2;
const gpio_num_t DIN_PIN = GPIO_NUM_3;
#endif
#endif
#endif

// Number of devices MAXIM 7219 / 7221 in the chain
const uint8_t ChainLength = 3;

// Time between two display update
const TickType_t DelayBetweenUpdates = pdMS_TO_TICKS(1000);



// Handle to the MAXIM 7219 / 7221 driver
led_driver_maxim7219_handle_t led_maxim7219_handle = NULL;



void app_main(void) {
    // Configure SPI bus to communicate with MAXIM 7219 / 7221
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

    // Initialize the MAXIM 7219 / 7221 driver
    maxim7219_config_t maxim7219InitConfig = {
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
    ESP_LOGI(TAG, "Initialize MAX 7219 / 7221 driver");
    ESP_ERROR_CHECK(led_driver_max7219_init(&maxim7219InitConfig, &led_maxim7219_handle));
    // NOTE: On power on, the MAXIM 2719 / 7221 starts in shutdown mode - All blank, scan mode is 1 digit, no CODE B decode, intensity is minimum

    // Switch to 'test' mode - This turns all segments on all displays ON at maximum intensity
    ESP_LOGI(TAG, "Set Test mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_TEST_MODE));

    // Configure scan limit on all devices
    ESP_LOGI(TAG, "Configure scan limit to all digits (8)");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_scan_limit(led_maxim7219_handle, 8));

    // Configure decode mode to 'decode for all digits'
    ESP_LOGI(TAG, "Configure decode for Code B on all digits in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_decode(led_maxim7219_handle, MAXIM7219_CODE_B_DECODE_ALL));

    // Set intensity on all devices - MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_2 is dim
    ESP_LOGI(TAG, "Set intensity to 'MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_2' on all devices in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_intensity(led_maxim7219_handle, MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_2));

    // Reset all digits to 'blank' for a clean visual effect - We use MAXIM7219_CODE_B_FONT_BLANK since we configured Code B decode
    // When the MAXIM 7219 / 7221 is put in test mode, it preserves whatever digits were programmed before
    // If no digits were programmed before entering test mode, the MAXIM 7219 / 7221 will load '8' in all digits
    ESP_LOGI(TAG, "Set all digits to blank");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain(led_maxim7219_handle, MAXIM7219_CODE_B_FONT_BLANK));


    // Hold 'test' mode for a little while
    vTaskDelay(DelayBetweenUpdates);

    // Switch to 'normal' mode so digits can be displayed and hold 'all blank' for a little while
    ESP_LOGI(TAG, "Set Normal mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_NORMAL_MODE));
    vTaskDelay(DelayBetweenUpdates);


    // Display '8' sequentially on all digits of all devices
    for (uint8_t chainId = 1; chainId <= ChainLength; chainId++) {
        for (uint8_t digitId = MAXIM7219_MIN_DIGIT; digitId <= MAXIM7219_MAX_DIGIT; digitId++) {
            ESP_LOGI(TAG, "Device %d: Set digit index %d to 'MAXIM7219_CODE_B_FONT_8'", chainId, digitId);
            ESP_ERROR_CHECK(led_driver_max7219_set_digit(led_maxim7219_handle, chainId, digitId, MAXIM7219_CODE_B_FONT_8));
            vTaskDelay(DelayBetweenUpdates);
        }
    }
    
    vTaskDelay(2 * DelayBetweenUpdates);

    uint8_t digits[] = {
        MAXIM7219_CODE_B_FONT_2,
        MAXIM7219_CODE_B_FONT_3,
        MAXIM7219_CODE_B_FONT_MINUS,
        MAXIM7219_CODE_B_FONT_H,
        MAXIM7219_CODE_B_FONT_P
    };
    const uint8_t digitCount = sizeof(digits) / sizeof(digits[0]);


    uint8_t startDevice = 1;
    uint8_t startDigit = 3;

    do {
        ESP_ERROR_CHECK(led_driver_max7219_set_digits(led_maxim7219_handle, startDevice, startDigit, digits, digitCount));

        for (uint8_t digit = 0; digit < digitCount; digit++) {
            digits[digit]++;
            if (digits[digit] > MAXIM7219_CODE_B_FONT_BLANK) {
                digits[digit] = MAXIM7219_CODE_B_FONT_0;
            }
        }

        vTaskDelay(DelayBetweenUpdates);
    } while (true);

    // Shutdown MAXIM 7219 / 7221 driver and SPI bus
    ESP_ERROR_CHECK(led_driver_max7219_free(led_maxim7219_handle));
    ESP_ERROR_CHECK(spi_bus_free(SPI_HOSTID));
}
