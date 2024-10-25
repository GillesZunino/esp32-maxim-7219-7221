// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <esp_check.h>

#include "maxim_7219_7221.h"

const char* TAG = "maxim72xx_main";

//
// NOTE: For maximum performance, prefer IO MUX over GPIO Matrix routing
//  * When using GPIO Matrix routing, the SPI bus speed is limited to 20 MHz and it may be necessary to adjust spi_device_interface_config_t::input_delay_ns (MISO_INPUT_DELAY_NANO_SECONDS below)
//  * See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html#gpio-matrix-routing
//
const int MISO_INPUT_DELAY_NANO_SECONDS = 60;


// SPI Host ID
const spi_host_device_t SPI_HOSTID = SPI2_HOST;

// SPI Pins
const gpio_num_t CS_PIN = GPIO_NUM_19;
const gpio_num_t SCLK_PIN = GPIO_NUM_18;
const gpio_num_t MOSI_PIN = GPIO_NUM_16;



void app_main(void) {
    // Configure SPI bus to communicate with MAXIM 72119 / 72121
    spi_bus_config_t spiBusConfig = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = SCLK_PIN,

        .data2_io_num = GPIO_NUM_NC,
        .data3_io_num = GPIO_NUM_NC,

        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOSTID, &spiBusConfig, SPI_DMA_CH_AUTO));

    
    const TickType_t DelayBetweenUpdates = pdMS_TO_TICKS(1000);
    do {
        vTaskDelay(DelayBetweenUpdates);
    } while (true);

    // Shutdown MAXIM 7219 / 7221 driver and SPI bus
    ESP_ERROR_CHECK(spi_bus_free(SPI_HOSTID));
}
