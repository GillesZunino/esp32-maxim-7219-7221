// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include <string.h>

#include <esp_check.h>

#include "maxim-7219-7221.h"


static const char* LedDriverMaxim7219LogTag = "leddriver-maxim7219";



typedef struct led_driver_maxim7219 {
    maxim7219_spi_config_t spi_cfg;
    maxim7219_hw_config_t hw_config;
    spi_device_handle_t spi_device_handle;
} led_driver_maxim7219_t;

typedef struct maxim7219_command {
    maxim7219_address_t address;
    uint8_t data;
}  __attribute__((packed)) maxim7219_command_t;


static esp_err_t led_driver_max7219_send_private(led_driver_maxim7219_handle_t handle, const maxim7219_command_t* const data, uint16_t length);

static esp_err_t check_driver_configuration_private(const maxim7219_config_t* config);
static esp_err_t check_maxim_handle_private(led_driver_maxim7219_handle_t handle);
static esp_err_t check_maxim_chain_id_private(led_driver_maxim7219_handle_t handle, uint8_t chainId);


esp_err_t led_driver_max7219_init(const maxim7219_config_t* config, led_driver_maxim7219_handle_t* handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Always clear return values even if we later fail
    *handle = NULL;

    // Check configuration
    ESP_RETURN_ON_ERROR(check_driver_configuration_private(config), LedDriverMaxim7219LogTag, "%s() Invalid configuration", __func__);

    // Allocate space for our handle
    led_driver_maxim7219_t* pLedMax7219 = heap_caps_calloc(1, sizeof(led_driver_maxim7219_t), MALLOC_CAP_DEFAULT);
    if (pLedMax7219 == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // Add an SPI device on the given bus - We accept the SPI bus configuration as is
    spi_device_interface_config_t spiDeviceInterfaceConfig = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,

        //
        // MAXIM 7219 (and 7221) both use Clock Polarity (CPOL) 0 and Clock Phase (CPHA) 0
        // * MAXIM 7219 is not SPI compliant: data is shifted into internal registers on CLK rising edges regardless of the state of LOAD
        // * MAXIM 7221 is SPI compliant: data will shift into internal registers only when /CS is low
        // For both devices, data will latch on the rising edge of /CS or LOAD
        //
        .mode = 0,

        .clock_source = config->spi_cfg.clock_source,
        .clock_speed_hz = config->spi_cfg.clock_speed_hz,
        .input_delay_ns = config->spi_cfg.input_delay_ns,

        .spics_io_num = config->spi_cfg.spics_io_num,

        .flags = 0,
        .queue_size = config->spi_cfg.queue_size
    };

    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(spi_bus_add_device(config->spi_cfg.host_id, &spiDeviceInterfaceConfig, &pLedMax7219->spi_device_handle), cleanup, LedDriverMaxim7219LogTag, "%s() Failed to spi_bus_add_device()", __func__);
    
    pLedMax7219->hw_config = config->hw_config;
    *handle = pLedMax7219;

    return ret;

cleanup:
    if (pLedMax7219 != NULL) {
        heap_caps_free(pLedMax7219);
    }

    return ret;
}

esp_err_t led_driver_max7219_free(led_driver_maxim7219_handle_t handle) {
    if (handle->spi_device_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Track the first error we encounter so we can return it to the caller - We do try to detach all aspects of the driver regardless of which step failed
    esp_err_t firstError = ESP_OK;

    // Put all MAXIM 7219 / 7221 cascaded on the chain in shutdown mode before freeing the driver
    esp_err_t err = led_driver_max7219_set_chain_mode(handle, MAXIM7219_SHUTDOWN_MODE);
    if (err != ESP_OK) {
        firstError = firstError == ESP_OK ? err : firstError;
        ESP_LOGW(LedDriverMaxim7219LogTag, "%s() Failed to set MAXIM 7219/7221 in shutdown mode (%d)", __func__, err);
    }

    // Remove the device from the bus
    err = spi_bus_remove_device(handle->spi_device_handle);
    if (err != ESP_OK) {
        firstError = firstError == ESP_OK ? err : firstError;
        ESP_LOGW(LedDriverMaxim7219LogTag, "%s() Failed to remove MAXIM 7219/7221 from SPI bus (%d)", __func__, err);
    }

    // Release memory
    heap_caps_free(handle);
    
    return firstError;
}

esp_err_t led_driver_max7219_set_chain_mode(led_driver_maxim7219_handle_t handle, maxim7219_mode_t mode) {
    ESP_RETURN_ON_ERROR(check_maxim_handle_private(handle), LedDriverMaxim7219LogTag, "%s() Invalid handle", __func__);

    uint16_t length = 0;
    maxim7219_command_t* buffer = NULL;

    switch (mode) {
        case MAXIM7219_SHUTDOWN_MODE:
        case MAXIM7219_NORMAL_MODE: {
            length = 2 * handle->hw_config.chain_length * sizeof(maxim7219_command_t);
            buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
            if (buffer != NULL) {
                for (uint8_t chipIndex = 0; chipIndex < 2 * handle->hw_config.chain_length; chipIndex += 2) {
                    // First, leave test mode by sending |MAXIM7219_TEST_ADDRESS|0| to all devices
                    buffer[chipIndex].address = MAXIM7219_TEST_ADDRESS;
                    buffer[chipIndex].data = 0;

                    // Then enter normal or shutdown mode by sending |MAXIM7219_SHUTDOWN_ADDRESS|<0 or 1>| to all devices
                    buffer[chipIndex + 1].address = MAXIM7219_SHUTDOWN_ADDRESS;
                    buffer[chipIndex + 1].data = mode == MAXIM7219_SHUTDOWN_MODE ? 0 : 1;
                }
            } else {
                return ESP_ERR_NO_MEM;
            }
        }
        break;
        case MAXIM7219_TEST_MODE: {
            // Send |MAXIM7219_TEST_ADDRESS|1| to all devices
            length = handle->hw_config.chain_length * sizeof(maxim7219_command_t);
            buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
            if (buffer != NULL) {
                for (uint8_t chipIndex = 0; chipIndex < handle->hw_config.chain_length; chipIndex++) {
                    buffer[chipIndex].address = MAXIM7219_TEST_ADDRESS;
                    buffer[chipIndex].data = 1;
                }
            } else {
                return ESP_ERR_NO_MEM;
            }
        }
        break;

        default:
            return ESP_ERR_INVALID_ARG;
    }

    // Transmit to the device - There is no data to read back
    esp_err_t err = led_driver_max7219_send_private(handle, buffer, length);

    if (buffer != NULL) {
        heap_caps_free(buffer);
    }

    return err;
}

esp_err_t led_driver_max7219_set_mode(led_driver_maxim7219_handle_t handle, uint8_t chainId, maxim7219_mode_t mode) {
    ESP_RETURN_ON_ERROR(check_maxim_handle_private(handle), LedDriverMaxim7219LogTag, "%s() Invalid handle", __func__);
    ESP_RETURN_ON_ERROR(check_maxim_chain_id_private(handle, chainId), LedDriverMaxim7219LogTag, "%s() Invalid chain ID", __func__);

    uint16_t length = 0;
    maxim7219_command_t* buffer = NULL;

    switch (mode) {
        case MAXIM7219_SHUTDOWN_MODE:
        case MAXIM7219_NORMAL_MODE: {
            length = 2 * handle->hw_config.chain_length * sizeof(maxim7219_command_t);
            buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
            if (buffer != NULL) {
                // The array is initialized to 0 which means .address is already set to MAXIM7219_NOOP_ADDRESS
                // Only populate the command for the requested item in the chain
                uint8_t chipIndex = 2 * (chainId - 1);

                // First, leave test mode by sending |MAXIM7219_TEST_ADDRESS|0| to all devices
                buffer[chipIndex].address = MAXIM7219_TEST_ADDRESS;
                buffer[chipIndex].data = 0;

                // Then enter normal or shutdown mode by sending |MAXIM7219_SHUTDOWN_ADDRESS|<0 or 1>| to all devices
                buffer[chipIndex + 1].address = MAXIM7219_SHUTDOWN_ADDRESS;
                buffer[chipIndex + 1].data = mode == MAXIM7219_SHUTDOWN_MODE ? 0 : 1;
            } else {
                return ESP_ERR_NO_MEM;
            }
        }
        break;

        case MAXIM7219_TEST_MODE: {
            length = 1 * handle->hw_config.chain_length * sizeof(maxim7219_command_t);
            buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
            if (buffer != NULL) {
                // The array is initialized to 0 which means .address is already set to MAXIM7219_NOOP_ADDRESS
                // Only populate the command for the requested item in the chain
                uint8_t chipIndex = 1 * (chainId - 1);

            // Send |MAXIM7219_TEST_ADDRESS|1|
                buffer[chipIndex].address = MAXIM7219_TEST_ADDRESS;
                buffer[chipIndex].data = 1;
            } else {
                return ESP_ERR_NO_MEM;
            }
        }
        break;

        default:
            return ESP_ERR_INVALID_ARG;
    }

    // Transmit to the device - There is no data to read back
    esp_err_t err = led_driver_max7219_send_private(handle, buffer, length);

    if (buffer != NULL) {
        heap_caps_free(buffer);
    }

    return err;
}

esp_err_t led_driver_max7219_configure_chain_decode(led_driver_maxim7219_handle_t handle, maxim7219_decode_mode_t decodeMode) {
    ESP_RETURN_ON_ERROR(check_maxim_handle_private(handle), LedDriverMaxim7219LogTag, "%s() Invalid handle", __func__);

    uint16_t length = 1 * handle->hw_config.chain_length * sizeof(maxim7219_command_t);
    maxim7219_command_t* buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
    if (buffer != NULL) {
        // Send |MAXIM7219_DECODE_MODE_ADDRESS|<mode>| to all chips
        for (uint8_t chipIndex = 0; chipIndex < handle->hw_config.chain_length; chipIndex++) {
            buffer[chipIndex].address = MAXIM7219_DECODE_MODE_ADDRESS;
            buffer[chipIndex].data = decodeMode;
        }
    } else {
        return ESP_ERR_NO_MEM;
    }

    // Transmit to the device - There is no data to read back
    return led_driver_max7219_send_private(handle, buffer, length);
}

esp_err_t led_driver_max7219_configure_decode(led_driver_maxim7219_handle_t handle, uint8_t chainId, maxim7219_decode_mode_t decodeMode) {
    ESP_RETURN_ON_ERROR(check_maxim_handle_private(handle), LedDriverMaxim7219LogTag, "%s() Invalid handle", __func__);
    ESP_RETURN_ON_ERROR(check_maxim_chain_id_private(handle, chainId), LedDriverMaxim7219LogTag, "%s() Invalid chain ID", __func__);

    uint16_t length = 1 * handle->hw_config.chain_length * sizeof(maxim7219_command_t);
    maxim7219_command_t*buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
    if (buffer != NULL) {
        // The array is initialized to 0 which means .address is already set to MAXIM7219_NOOP_ADDRESS
        // Only populate the command for the requested item in the chain
        uint8_t chipIndex = 1 * (chainId - 1);

        // Send |MAXIM7219_DECODE_MODE_ADDRESS|<mode>|
        buffer[chipIndex].address = MAXIM7219_DECODE_MODE_ADDRESS;
        buffer[chipIndex].data = decodeMode;
    } else {
        return ESP_ERR_NO_MEM;
    }

    // Transmit to the device - There is no data to read back
    esp_err_t err =  led_driver_max7219_send_private(handle, buffer, length);

    if (buffer != NULL) {
        heap_caps_free(buffer);
    }

    return err;
}

esp_err_t led_driver_max7219_set_chain_intensity(led_driver_maxim7219_handle_t handle, maxim7219_intensity_t intensity) {
    ESP_RETURN_ON_ERROR(check_maxim_handle_private(handle), LedDriverMaxim7219LogTag, "%s() Invalid handle", __func__);

    uint16_t length = 1 * handle->hw_config.chain_length * sizeof(maxim7219_command_t);
    maxim7219_command_t* buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
    if (buffer != NULL) {
        // Send |MAXIM7219_INTENSITY_ADDRESS|<intensity>| to all chips
        for (uint8_t chipIndex = 0; chipIndex < handle->hw_config.chain_length; chipIndex++) {
            buffer[chipIndex].address = MAXIM7219_INTENSITY_ADDRESS;
            buffer[chipIndex].data = intensity;
        }
    } else {
        return ESP_ERR_NO_MEM;
    }

    // Transmit to the device - There is no data to read back
    esp_err_t err =  led_driver_max7219_send_private(handle, buffer, length);

    if (buffer != NULL) {
        heap_caps_free(buffer);
    }

    return err;
}

esp_err_t led_driver_max7219_set_intensity(led_driver_maxim7219_handle_t handle, uint8_t chainId, maxim7219_intensity_t intensity) {
    ESP_RETURN_ON_ERROR(check_maxim_handle_private(handle), LedDriverMaxim7219LogTag, "%s() Invalid handle", __func__);
    ESP_RETURN_ON_ERROR(check_maxim_chain_id_private(handle, chainId), LedDriverMaxim7219LogTag, "%s() Invalid chain ID", __func__);

    uint16_t length = 1 * handle->hw_config.chain_length * sizeof(maxim7219_command_t);
    maxim7219_command_t*buffer = heap_caps_calloc(1, length, MALLOC_CAP_DEFAULT);
    if (buffer != NULL) {
        // The array is initialized to 0 which means .address is already set to MAXIM7219_NOOP_ADDRESS
        // Only populate the command for the requested item in the chain
        uint8_t chipIndex = 1 * (chainId - 1);

        // Send |MAXIM7219_INTENSITY_ADDRESS|<intensity>|
        buffer[chipIndex].address = MAXIM7219_INTENSITY_ADDRESS;
        buffer[chipIndex].data = intensity;
    } else {
        return ESP_ERR_NO_MEM;
    }

    // Transmit to the device - There is no data to read back
    esp_err_t err =  led_driver_max7219_send_private(handle, buffer, length);

    if (buffer != NULL) {
        heap_caps_free(buffer);
    }

    return err;
}


static esp_err_t led_driver_max7219_send_private(led_driver_maxim7219_handle_t handle, const maxim7219_command_t* const data, uint16_t length) {
    bool useTxData = length <= 4;
    uint8_t transactionLengthInBytes = length * sizeof(maxim7219_command_t);
    spi_transaction_t spiTransaction = {
        .flags = useTxData ? SPI_TRANS_USE_TXDATA : 0,
        .length = transactionLengthInBytes * 8,
        .rxlength = 0
    };
    if (useTxData) {
        memcpy(spiTransaction.tx_data, data, length);
    } else {
        spiTransaction.tx_buffer = data;
    }

    return spi_device_transmit(handle->spi_device_handle, &spiTransaction);
}

static esp_err_t check_driver_configuration_private(const maxim7219_config_t* config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check SPI configuration - Clock speed must be non zero and up to 10 MHz
    if ((config->spi_cfg.clock_speed_hz <= 0) || (config->spi_cfg.clock_speed_hz > 10 * 1000000)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check SPI configuration - /CS (LOAD) must be specified as it is used to latch data
    if (config->spi_cfg.spics_io_num == GPIO_NUM_NC) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check hardware configuration - Scan Limit must be 1 to 8
    if ((config->hw_config.scan_limit < 1) || (config->hw_config.scan_limit > 8)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check hardware configuration - Chain length must be at least 1 and less than 255
    if ((config->hw_config.chain_length < 1) || (config->hw_config.chain_length > 254)) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check hardware configuration - Chip type must be either MAXIM_7219_TYPE or MAXIM_7221_TYPE
    if ((config->hw_config.chip_type != MAXIM_7219_TYPE) && (config->hw_config.chip_type != MAXIM_7221_TYPE)) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

static esp_err_t check_maxim_handle_private(led_driver_maxim7219_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->spi_device_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

static esp_err_t check_maxim_chain_id_private(led_driver_maxim7219_handle_t handle, uint8_t chainId) {
    return (chainId >= 1) && (chainId <= handle->hw_config.chain_length) ? ESP_OK : ESP_ERR_INVALID_ARG;
}