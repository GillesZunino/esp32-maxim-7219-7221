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
//  * When using GPIO Matrix routing, the SPI bus speed is limited to 20 MHz and it may be necessary to adjust spi_device_interface_config_t::input_delay_ns (MISO_INPUT_DELAY_NANO_SECONDS below)
//  * See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html#gpio-matrix-routing
//
const int MISO_INPUT_DELAY_NANO_SECONDS = 60;


// SPI Host ID
const spi_host_device_t SPI_HOSTID = SPI2_HOST;

// SPI Pins
const gpio_num_t CS_LOAD_PIN = GPIO_NUM_19;
const gpio_num_t CLK_PIN = GPIO_NUM_18;
const gpio_num_t DIN_PIN = GPIO_NUM_16;

// Number of devices MAXIM 7219 / 7221 in the chain
const uint8_t ChainLength = 1;


// Handle to the MAXIM 7219 / 7221 driver
led_driver_maxim7219_handle_t led_maxim7219_handle = NULL;

// TIme in between two display digit changes
const TickType_t DelayBetweenUpdates = pdMS_TO_TICKS(1000);


void app_main(void) {
    // Configure SPI bus to communicate with MCP2515
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

            .input_delay_ns = MISO_INPUT_DELAY_NANO_SECONDS,

            .spics_io_num = CS_LOAD_PIN,
            .queue_size = 8
        },
        .hw_config = {
            .chain_length = ChainLength,
            .device_type = MAXIM_7219_TYPE
        }
    };
    ESP_LOGI(TAG, "Initialize MCP2515 driver");
    ESP_ERROR_CHECK(led_driver_max7219_init(&maxim7219InitConfig, &led_maxim7219_handle));
    
    // Configure scan limits on all devices
    ESP_LOGI(TAG, "Configure scan limits to all digits (8)");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_scan_limit(led_maxim7219_handle, 8));

    // Configure decode mode to 'decode for all digits'
    ESP_LOGI(TAG, "Configure decode for Code B on all digits in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_decode(led_maxim7219_handle, MAXIM7219_CODE_B_DECODE_ALL));

    // Set intensity on all devices - MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_4 is dim
    ESP_LOGI(TAG, "Set intensity to 'MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_4' on all devices in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_intensity(led_maxim7219_handle, MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_4));

    // Switch to 'test' mode (The MAXIM 2719 / 2722 starts in shutdown mode by default)
    // NOTE: Test mode always ignore the configured itensity
    ESP_LOGI(TAG, "Set Test mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_TEST_MODE));
    vTaskDelay(DelayBetweenUpdates);

    // Switch to 'normal' mode (The MAXIM 2719 / 2722 starts in shutdown mode by default)
    ESP_LOGI(TAG, "Set Normal mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_NORMAL_MODE));


    const TickType_t DelayBetweenUpdates = pdMS_TO_TICKS(1000);

    // Display '8' sequentially on all digits of all devices
    for (uint8_t chainId = 1; chainId <= ChainLength; chainId++) {
        for (uint8_t digitId = 1; digitId <= 8; digitId++) {
            ESP_LOGI(TAG, "Device %d: Set digit index %d to 'MAXIM7219_CODE_B_FONT_8'", chainId, digitId);
            ESP_ERROR_CHECK(led_driver_max7219_set_digit(led_maxim7219_handle, chainId, digitId, MAXIM7219_CODE_B_FONT_8));
            vTaskDelay(DelayBetweenUpdates);
        }
    }
    
    do {
        vTaskDelay(DelayBetweenUpdates);
    } while (true);

    // Shutdown MAXIM 7219 / 7221 driver and SPI bus
    ESP_ERROR_CHECK(led_driver_max7219_free(led_maxim7219_handle));
    ESP_ERROR_CHECK(spi_bus_free(SPI_HOSTID));
}
