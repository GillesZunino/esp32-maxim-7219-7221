// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <esp_check.h>

#include "maxim_7219_7221.h"

const char* TAG = "max72[19|21]_basic";

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

// Number of devices MAXIM 7219 / 7221 in the chain
const uint8_t ChainLength = 1;

// Time between two display update
const TickType_t DelayBetweenUpdates = pdMS_TO_TICKS(1000);



// Handle to the MAXIM 7219 / 7221 driver
led_driver_maxim7219_handle_t led_maxim7219_handle = NULL;



void app_main(void) {
    // Configure SPI bus to communicate with MAXIM 72119 / 72121
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

    // Configure scan limit on all devices
    ESP_LOGI(TAG, "Configure scan limit to all digits (8)");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_scan_limit(led_maxim7219_handle, 8));


    const uint8_t DeviceChainId = 1;

    do {
        // Ensure we are in shutdown mode before changing configuration
        ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_SHUTDOWN_MODE));

        // Configure decode mode to 'decode for all digits'
        ESP_LOGI(TAG, "Configure decode for Code B on all digits in the chain");
        ESP_ERROR_CHECK(led_driver_max7219_configure_chain_decode(led_maxim7219_handle, MAXIM7219_CODE_B_DECODE_ALL));

        // Reset all digits to 'blank' for a clean visual effect - We use MAXIM7219_CODE_B_FONT_BLANK since we configured Code B decode
        // When the MAXIM 7219 / 7221 is put in test mode, it preserves whatever digits were programmed before
        // If no digits were programmed before entering test mode, the MAXIM 7219 / 7221 will load '8' in all digits
        ESP_LOGI(TAG, "Set all digits to blank");
        ESP_ERROR_CHECK(led_driver_max7219_set_chain(led_maxim7219_handle, MAXIM7219_CODE_B_FONT_BLANK));

        // Switch to 'normal' mode so digits can be displayed and hold 'all blank' for a little while
        ESP_LOGI(TAG, "Set Normal mode");
        ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_NORMAL_MODE));

        // Cycle all digits through all Code B symbols
        for (uint8_t digitId = MAXIM7219_MIN_DIGIT; digitId <= MAXIM7219_MAX_DIGIT; digitId++) {
            for (maxim7219_code_b_font_t symbol = MAXIM7219_CODE_B_FONT_0; symbol <= MAXIM7219_CODE_B_FONT_BLANK; symbol++) {
                // Set the symbol on the specific digit - Also toggle the decimal point on / off as we go
                bool decimalOn = symbol % 2 == 0;
                maxim7219_code_b_font_t symbolWithDecimal = decimalOn ? symbol | MAXIM7219_CODE_B_DP_MASK : symbol;
                ESP_LOGI(TAG, "Device %d: Set digit index %d to '%d' - Decimal '%s'", DeviceChainId, digitId, symbolWithDecimal, decimalOn ? "ON" : "OFF");
                ESP_ERROR_CHECK(led_driver_max7219_set_digit(led_maxim7219_handle, DeviceChainId, digitId, symbolWithDecimal));
                vTaskDelay(DelayBetweenUpdates);
            }
        }


        vTaskDelay(2 * DelayBetweenUpdates);
        

        // Configure decode mode to 'no decode for all digits' - We disable all digits (shutdown mode) before changing configuration
        ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_SHUTDOWN_MODE));

        ESP_LOGI(TAG, "Configure decode to 'no decode' for all digits in the chain and blank the chain");
        ESP_ERROR_CHECK(led_driver_max7219_configure_chain_decode(led_maxim7219_handle, MAXIM7219_CODE_B_DECODE_NONE));
        ESP_ERROR_CHECK(led_driver_max7219_set_chain(led_maxim7219_handle, MAXIM7219_CUSTOM_BLANK));

        ESP_ERROR_CHECK(led_driver_max7219_set_chain_mode(led_maxim7219_handle, MAXIM7219_NORMAL_MODE));

        // Make all custom symbols accessible via indexing in an array
        const maxim7219_custom_font_t AllCustomSymbols[] = {
            MAXIM7219_CUSTOM_0,
            MAXIM7219_CUSTOM_1,
            MAXIM7219_CUSTOM_2,
            MAXIM7219_CUSTOM_3,
            MAXIM7219_CUSTOM_4,
            MAXIM7219_CUSTOM_5,
            MAXIM7219_CUSTOM_6,
            MAXIM7219_CUSTOM_7,
            MAXIM7219_CUSTOM_8,
            MAXIM7219_CUSTOM_9,
            MAXIM7219_CUSTOM_A,
            MAXIM7219_CUSTOM_C,
            MAXIM7219_CUSTOM_E,
            MAXIM7219_CUSTOM_F,
            MAXIM7219_CUSTOM_H,
            MAXIM7219_CUSTOM_J,
            MAXIM7219_CUSTOM_L,
            MAXIM7219_CUSTOM_P,
            MAXIM7219_CUSTOM_U,
            MAXIM7219_CUSTOM_MINUS,
            MAXIM7219_CUSTOM_BLANK
        };

        // Cycle all digits through all custom symbols
        for (uint8_t digitId = MAXIM7219_MIN_DIGIT; digitId <= MAXIM7219_MAX_DIGIT; digitId++) {
            for (uint8_t symbolIndex = 0; symbolIndex < sizeof(AllCustomSymbols) / sizeof(AllCustomSymbols[0]); symbolIndex++) {
                // Set the symbol on the specific digit - Also toggle the decimal point on / off as we go - BLANK has also decimal point off
                maxim7219_custom_font_t symbol = AllCustomSymbols[symbolIndex];
                bool decimalOn = ((symbolIndex % 2) == 0) && (symbol != MAXIM7219_CUSTOM_BLANK);
                maxim7219_custom_font_t symbolWithDecimal = decimalOn ? symbol | MAXIM7219_SEGMENT_DP : symbol;
                ESP_LOGI(TAG, "Device %d: Set digit index %d to '%d' - Decimal '%s'", DeviceChainId, digitId, symbolWithDecimal, decimalOn ? "ON" : "OFF");
                ESP_ERROR_CHECK(led_driver_max7219_set_digit(led_maxim7219_handle, DeviceChainId, digitId, symbolWithDecimal));
                vTaskDelay(DelayBetweenUpdates);
            }
        }

        vTaskDelay(2 * DelayBetweenUpdates);
    } while (true);

    // Shutdown MAXIM 7219 / 7221 driver and SPI bus
    ESP_ERROR_CHECK(led_driver_max7219_free(led_maxim7219_handle));
    ESP_ERROR_CHECK(spi_bus_free(SPI_HOSTID));
}
