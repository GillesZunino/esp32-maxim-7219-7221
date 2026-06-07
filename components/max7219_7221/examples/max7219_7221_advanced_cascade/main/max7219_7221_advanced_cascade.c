// -----------------------------------------------------------------------------------
// Copyright 2026, Gilles Zunino
// -----------------------------------------------------------------------------------

#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <esp_check.h>

#include "max7219_7221.h"

const char* TAG = "max72[19|21]_cascade";

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
#else
#if CONFIG_IDF_TARGET_ESP32H2
const gpio_num_t CS_LOAD_PIN = GPIO_NUM_12;
const gpio_num_t CLK_PIN = GPIO_NUM_22;
const gpio_num_t DIN_PIN = GPIO_NUM_25;
#endif
#endif
#endif
#endif

// Number of devices MAX7219 / MAX7221 in the chain
const uint8_t ChainLength = 3;

// Time between two display updates
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

    // Configure decode mode to 'decode for all digits'
    ESP_LOGI(TAG, "Configure decode for Code B on all digits in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_configure_chain_decode(led_max7219_handle, MAX7219_CODE_B_DECODE_ALL));

    // Set intensity on all devices - MAX7219_INTENSITY_DUTY_CYCLE_STEP_1 is dimmest
    ESP_LOGI(TAG, "Set intensity to 'MAX7219_INTENSITY_DUTY_CYCLE_STEP_2' on all devices in the chain");
    ESP_ERROR_CHECK(led_driver_max7219_set_chain_intensity(led_max7219_handle, MAX7219_INTENSITY_DUTY_CYCLE_STEP_2));


    // Device 1: Display 1 to 8 digits
    const uint8_t Device1DigitsToDisplay[] = {
        MAX7219_CODE_B_1,
        MAX7219_CODE_B_2,
        MAX7219_CODE_B_3,
        MAX7219_CODE_B_4,
        MAX7219_CODE_B_5,
        MAX7219_CODE_B_6,
        MAX7219_CODE_B_7,
        MAX7219_CODE_B_8
    };
    ESP_ERROR_CHECK(led_driver_max7219_set_digits(led_max7219_handle, 1, 1, Device1DigitsToDisplay, sizeof(Device1DigitsToDisplay) / sizeof(Device1DigitsToDisplay[0])));

    // Set device 1 to 'normal' mode so digits can be displayed
    ESP_LOGI(TAG, "Device 1 -> Set Normal mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_mode(led_max7219_handle, 1, MAX7219_NORMAL_MODE));
    
    // Device 2: Display '--------' or test mode depending on the mode
    const uint8_t Device2SymbolsToDisplay[] = {
        MAX7219_CODE_B_MINUS,
        MAX7219_CODE_B_MINUS,
        MAX7219_CODE_B_MINUS,
        MAX7219_CODE_B_MINUS,
        MAX7219_CODE_B_MINUS,
        MAX7219_CODE_B_MINUS,
        MAX7219_CODE_B_MINUS,
        MAX7219_CODE_B_MINUS
    };
    ESP_ERROR_CHECK(led_driver_max7219_set_digits(led_max7219_handle, 2, 1, Device2SymbolsToDisplay, sizeof(Device2SymbolsToDisplay) / sizeof(Device2SymbolsToDisplay[0])));

    // Set device 2 to 'normal' mode so digits can be displayed
    ESP_LOGI(TAG, "Device 2 -> Set Normal mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_mode(led_max7219_handle, 2, MAX7219_NORMAL_MODE));
    
    // Device 3: Display '  HELO  ' or shutdown depending on the mode
    const uint8_t Device3SymbolsToDisplay[] = {
        MAX7219_CODE_B_BLANK,
        MAX7219_CODE_B_BLANK,
        MAX7219_CODE_B_0,
        MAX7219_CODE_B_L,
        MAX7219_CODE_B_E,
        MAX7219_CODE_B_H,
        MAX7219_CODE_B_BLANK,
        MAX7219_CODE_B_BLANK
    };
    ESP_ERROR_CHECK(led_driver_max7219_set_digits(led_max7219_handle, 3, 1, Device3SymbolsToDisplay, sizeof(Device3SymbolsToDisplay) / sizeof(Device3SymbolsToDisplay[0])));

    // Set device 3 to 'normal' mode so digits can be displayed
    ESP_LOGI(TAG, "Device 3 -> Set Normal mode");
    ESP_ERROR_CHECK(led_driver_max7219_set_mode(led_max7219_handle, 3, MAX7219_NORMAL_MODE));

    // Hold in the current configuration for a little while
    vTaskDelay(3 * DelayBetweenUpdates);

    max7219_mode_t device2_mode = MAX7219_TEST_MODE;
    max7219_mode_t device3_mode = MAX7219_SHUTDOWN_MODE;

    do {
        // Set device 2 to test mode or normal mode
        ESP_ERROR_CHECK(led_driver_max7219_set_mode(led_max7219_handle, 2, device2_mode));
        device2_mode = device2_mode == MAX7219_TEST_MODE ? MAX7219_NORMAL_MODE : MAX7219_TEST_MODE;

        vTaskDelay(DelayBetweenUpdates);

        // Set device 3 to shutdown mode or normal mode
        ESP_ERROR_CHECK(led_driver_max7219_set_mode(led_max7219_handle, 3, device3_mode));
        device3_mode = device3_mode == MAX7219_NORMAL_MODE ? MAX7219_SHUTDOWN_MODE : MAX7219_NORMAL_MODE;

        vTaskDelay(DelayBetweenUpdates);
    } while (true);

    // Shutdown MAX7219 / MAX7221 driver and SPI bus
    ESP_ERROR_CHECK(led_driver_max7219_free(led_max7219_handle));
    ESP_ERROR_CHECK(spi_bus_free(SPI_HOSTID));
}
