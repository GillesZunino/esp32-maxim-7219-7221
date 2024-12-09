// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <esp_check.h>

#include "max7219_7221.h"

const char* TAG = "max72[19|21]_decode";

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
const uint8_t ChainLength = 1;

// Time between two display update
const TickType_t DelayBetweenUpdates = pdMS_TO_TICKS(1000);



// Handle to the MAX7219 / MAX7221 driver
led_driver_max7219_handle_t led_max7219_handle = NULL;



void app_main(void) {
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

    // Configure decode mode to all even digits D|x|D|x|D|x|D|x
    max7219_decode_mode_t decodeDigits = MAX7219_CODE_B_DECODE_DIGIT_2 | MAX7219_CODE_B_DECODE_DIGIT_4 | MAX7219_CODE_B_DECODE_DIGIT_6 | MAX7219_CODE_B_DECODE_DIGIT_8;
    ESP_LOGI(TAG, "Configure decode for Code B to all even digits -> D|x|D|x|D|x|D|x");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_decode(led_max7219_handle, decodeDigits));

    // Set digits in mixed addressing mode: Code B and Direct Addressing
    const uint8_t StartDeviceChainId = 1;
    const uint8_t StartDigitId = 1;

    const uint8_t digitsToDisplay[] = {
        MAX7219_DIRECT_ADDRESSING_1,
        MAX7219_CODE_B_2,
        MAX7219_DIRECT_ADDRESSING_3,
        MAX7219_CODE_B_4,
        MAX7219_DIRECT_ADDRESSING_5,
        MAX7219_CODE_B_6,
        MAX7219_DIRECT_ADDRESSING_7,
        MAX7219_CODE_B_H
    };
    const uint8_t digitsCount = sizeof(digitsToDisplay) / sizeof(digitsToDisplay[0]);

    // Set digits on the display - This has a mix of Code B and Direct Addressing digits
    ESP_LOGI(TAG, "Start Device %d: - Start digit index %d", StartDeviceChainId, StartDigitId);
    ESP_ERROR_CHECK(led_driver_max7219_set_digits(led_max7219_handle, StartDeviceChainId, StartDigitId, digitsToDisplay, digitsCount));

    // Switch to 'normal' mode so digits can be displayed and hold 'all blank' for a little while
    ESP_LOGI(TAG, "Set Normal mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_max7219_handle, MAX7219_NORMAL_MODE));

    do {
        vTaskDelay(DelayBetweenUpdates);
    } while (true);

    // Shutdown MAX7219 / MAX7221  driver and SPI bus
    ESP_ERROR_CHECK(led_driver_max7219_free(led_max7219_handle));
    ESP_ERROR_CHECK(spi_bus_free(SPI_HOSTID));
}
