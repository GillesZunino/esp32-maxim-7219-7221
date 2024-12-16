// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include <string.h>

#include <esp_attr.h>
#include <esp_check.h>

#include "max7219_7221.h"


DRAM_ATTR static const char* LedDriverMax7219LogTag = "leddriver_max72[19|21]";


typedef enum {
    MAX7219_NOOP_ADDRESS = 0x00,
    MAX7219_DIGIT0_ADDRESS = 0x01,
    MAX7219_DIGIT1_ADDRESS = 0x02,
    MAX7219_DIGIT2_ADDRESS = 0x03,
    MAX7219_DIGIT3_ADDRESS = 0x04,
    MAX7219_DIGIT4_ADDRESS = 0x05,
    MAX7219_DIGIT5_ADDRESS = 0x06,
    MAX7219_DIGIT6_ADDRESS = 0x07,
    MAX7219_DIGIT7_ADDRESS = 0x08,
    MAX7219_DECODE_MODE_ADDRESS = 0x09,
    MAX7219_INTENSITY_ADDRESS = 0x0A,
    MAX7219_SCAN_LIMIT_ADDRESS = 0x0B,
    MAX7219_SHUTDOWN_ADDRESS = 0x0C,
    MAX7219_TEST_ADDRESS = 0x0F
} __attribute__ ((__packed__)) max7219_address_t;


typedef struct max7219_command {
    max7219_address_t address;
    uint8_t data;
}  __attribute__((packed)) max7219_command_t;

typedef struct max7219_chain_commands {
    bool use_inline_buffer;
    union {
        max7219_command_t commands_data[2];
        max7219_command_t* commands_buffer;
    };
} __attribute__((packed)) max7219_chain_commands_t;


typedef struct led_driver_max7219 {
    SemaphoreHandle_t mutex;
    max7219_hw_config_t hw_config;
    spi_device_handle_t spi_device_handle;
    max7219_chain_commands_t commands;
} led_driver_max7219_t;


static void free_driver_memory_private(led_driver_max7219_handle_t handle);

static inline __attribute__((always_inline)) max7219_command_t* get_command_buffer_private(led_driver_max7219_handle_t handle);
static esp_err_t send_chain_command_private(led_driver_max7219_handle_t handle, uint8_t chainId, const max7219_command_t cmd);
static esp_err_t led_driver_max7219_send_private(led_driver_max7219_handle_t handle, const max7219_command_t* const data, uint16_t commandsCount);

static esp_err_t check_driver_configuration_private(const max7219_config_t* config);
static esp_err_t check_max_handle_private(led_driver_max7219_handle_t handle);
static esp_err_t check_max_chain_id_private(led_driver_max7219_handle_t handle, uint8_t chainId);
static esp_err_t check_max_digit_private(led_driver_max7219_handle_t handle, uint8_t digit);
static esp_err_t check_bulk_symbols_array_length(led_driver_max7219_handle_t handle, uint8_t startChainId, uint8_t startDigitId, uint8_t digitCodesCount);


esp_err_t led_driver_max7219_init(const max7219_config_t* config, led_driver_max7219_handle_t* handle) {
    if (handle == NULL) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "handle must not be NULL");
#endif
        return ESP_ERR_INVALID_ARG;
    }
    
    // Always clear return values even if we later fail
    *handle = NULL;

    // Check configuration
    ESP_RETURN_ON_ERROR(check_driver_configuration_private(config), LedDriverMax7219LogTag, "Invalid configuration");

    // Allocate space for our handle
    led_driver_max7219_t* pLedMax7219 = heap_caps_calloc(1, sizeof(led_driver_max7219_t), MALLOC_CAP_DEFAULT);
    if (pLedMax7219 == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // Allocate space for the command buffer - Only allocate dynamic memory if the command buffer cannot fit in spi_transaction_t.tx_data which is a uint8_t[4]
    esp_err_t ret = ESP_OK;
    pLedMax7219->commands.use_inline_buffer = config->hw_config.chain_length * sizeof(max7219_command_t) <= sizeof(uint8_t[4]);
    if (!pLedMax7219->commands.use_inline_buffer) {
        pLedMax7219->commands.commands_buffer = heap_caps_calloc(config->hw_config.chain_length, sizeof(max7219_command_t), MALLOC_CAP_DMA);
        ESP_GOTO_ON_FALSE(pLedMax7219->commands.commands_buffer != NULL, ESP_ERR_NO_MEM, cleanup, LedDriverMax7219LogTag, "Could not allocate memory for command buffer");
    }

    // Initialize mutex for multi threading protection
    pLedMax7219->mutex = xSemaphoreCreateMutexWithCaps(MALLOC_CAP_DEFAULT);
    ESP_GOTO_ON_FALSE(pLedMax7219->mutex != NULL, ESP_ERR_NO_MEM, cleanup, LedDriverMax7219LogTag, "Could not allocate memory for mutex");

    // Add an SPI device on the given bus - We accept the SPI bus configuration as is
    spi_device_interface_config_t spiDeviceInterfaceConfig = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,

        //
        // MAX7219 (and 7221) both use Clock Polarity (CPOL) 0 and Clock Phase (CPHA) 0
        // * MAX7219 is not SPI compliant: data is shifted into internal registers on CLK rising edges regardless of the state of LOAD
        // * MAX7221 is SPI compliant: data will shift into internal registers only when /CS is low
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

    ESP_GOTO_ON_ERROR(spi_bus_add_device(config->spi_cfg.host_id, &spiDeviceInterfaceConfig, &pLedMax7219->spi_device_handle), cleanup, LedDriverMax7219LogTag, "Failed to spi_bus_add_device()");
    
    pLedMax7219->hw_config = config->hw_config;
    *handle = pLedMax7219;

    return ret;

cleanup:
    free_driver_memory_private(pLedMax7219);
    return ret;
}

esp_err_t led_driver_max7219_free(led_driver_max7219_handle_t handle) {
    if (handle->spi_device_handle == NULL) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "handle must not be NULL");
#endif
        return ESP_ERR_INVALID_STATE;
    }

    // Track the first error we encounter so we can return it to the caller - We do try to detach all aspects of the driver regardless of which step failed
    esp_err_t firstError = ESP_OK;

    // Put all MAX7219 / MAX7221 cascaded on the chain in shutdown mode before freeing the driver
    esp_err_t err = led_driver_max7219_set_chain_mode(handle, MAX7219_SHUTDOWN_MODE);
    if (err != ESP_OK) {
        firstError = firstError == ESP_OK ? err : firstError;
        ESP_LOGW(LedDriverMax7219LogTag, "Failed to set MAX7219/MAX7221 in shutdown mode (%d)", err);
    }

    // Remove the device from the bus
    err = spi_bus_remove_device(handle->spi_device_handle);
    if (err != ESP_OK) {
        firstError = firstError == ESP_OK ? err : firstError;
        ESP_LOGW(LedDriverMax7219LogTag, "Failed to remove MAX7219/MAX7221 from SPI bus (%d)", err);
    }

    // Release memory
    free_driver_memory_private(handle);    
    return firstError;
}

static void free_driver_memory_private(led_driver_max7219_handle_t handle) {
    if (handle != NULL) {
        if (handle->mutex != NULL) {
            vSemaphoreDeleteWithCaps(handle->mutex);
            handle->mutex = NULL;
        }

        if (!handle->commands.use_inline_buffer && (handle->commands.commands_buffer != NULL)) {
            heap_caps_free(handle->commands.commands_buffer);
            handle->commands.commands_buffer = NULL;
        }
        
        heap_caps_free(handle);
    }
} 

esp_err_t led_driver_max7219_configure_chain_decode(led_driver_max7219_handle_t handle, max7219_decode_mode_t decodeMode) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");

    // Send |MAX7219_DECODE_MODE_ADDRESS|<mode>| to all devices
    max7219_command_t command = { .address = MAX7219_DECODE_MODE_ADDRESS, .data = decodeMode };
    return send_chain_command_private(handle, 0, command);
}

esp_err_t led_driver_max7219_configure_decode(led_driver_max7219_handle_t handle, uint8_t chainId, max7219_decode_mode_t decodeMode) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");
    ESP_RETURN_ON_ERROR(check_max_chain_id_private(handle, chainId), LedDriverMax7219LogTag, "Invalid chain ID");

    // Send |MAX7219_DECODE_MODE_ADDRESS|<mode>| to the requested device
    max7219_command_t command = { .address = MAX7219_DECODE_MODE_ADDRESS, .data = decodeMode };
    return send_chain_command_private(handle, chainId, command);
}

esp_err_t led_driver_max7219_configure_chain_scan_limit(led_driver_max7219_handle_t handle, uint8_t digits) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");
    ESP_RETURN_ON_ERROR(check_max_digit_private(handle, digits), LedDriverMax7219LogTag, "Invalid digits");

    // Send |MAX7219_SCAN_LIMIT_ADDRESS|<digits - 1>| to all devices
    max7219_command_t command = { .address = MAX7219_SCAN_LIMIT_ADDRESS, .data = digits - 1 };
    return send_chain_command_private(handle, 0, command);
}

esp_err_t led_driver_max7219_configure_scan_limit(led_driver_max7219_handle_t handle, uint8_t chainId, uint8_t digits) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");
    ESP_RETURN_ON_ERROR(check_max_chain_id_private(handle, chainId), LedDriverMax7219LogTag, "Invalid chain ID");
    ESP_RETURN_ON_ERROR(check_max_digit_private(handle, digits), LedDriverMax7219LogTag, "Invalid digits");

    // Send |MAX7219_SCAN_LIMIT_ADDRESS|<digits - 1>| to the requested device
    max7219_command_t command = { .address = MAX7219_SCAN_LIMIT_ADDRESS, .data = digits - 1 };
    return send_chain_command_private(handle, chainId, command);
}



esp_err_t led_driver_max7219_set_chain_mode(led_driver_max7219_handle_t handle, max7219_mode_t mode) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");

    switch (mode) {
        case MAX7219_SHUTDOWN_MODE:
        case MAX7219_NORMAL_MODE: {
            ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_TIMEOUT, LedDriverMax7219LogTag, "Could not acquire mutex");

            // Take exclusive access of the SPI bus
            esp_err_t ret = ESP_OK;
            ESP_GOTO_ON_ERROR(spi_device_acquire_bus(handle->spi_device_handle, portMAX_DELAY), cleanup, LedDriverMax7219LogTag, "Unable to acquire SPI bus");

                max7219_command_t* buffer = get_command_buffer_private(handle);

                // Leave test mode (if on) by sending |MAX7219_TEST_ADDRESS|0| to all devices
                {
                    max7219_command_t command = { .address = MAX7219_TEST_ADDRESS, .data = 0 };
                    for (uint8_t deviceIndex = 0; deviceIndex < handle->hw_config.chain_length; deviceIndex++) {
                        buffer[deviceIndex] = command;
                    }
                    ESP_GOTO_ON_ERROR(led_driver_max7219_send_private(handle, buffer, handle->hw_config.chain_length), releasebus, LedDriverMax7219LogTag, "Failed to send commands to chain");
                }

                // Enter normal or shutdown mode by sending |MAX7219_SHUTDOWN_ADDRESS|<0 or 1>| to all devices
                {
                    max7219_command_t command = { .address = MAX7219_SHUTDOWN_ADDRESS, .data = mode == MAX7219_SHUTDOWN_MODE ? 0 : 1 };
                    for (uint8_t deviceIndex = 0; deviceIndex < handle->hw_config.chain_length; deviceIndex++) {
                        buffer[deviceIndex] = command;
                    }
                    ESP_GOTO_ON_ERROR(led_driver_max7219_send_private(handle, buffer, handle->hw_config.chain_length), releasebus, LedDriverMax7219LogTag, "Failed to send commands to chain");
                }
releasebus:
            // Release access to the SPI bus
            spi_device_release_bus(handle->spi_device_handle);

cleanup:
            // Release mutex
            if (xSemaphoreGive(handle->mutex) != pdTRUE) {
                ESP_LOGE(LedDriverMax7219LogTag, "Could not release mutex - Exiting without releasing mutex which may cause a deadlock later");
            }

            return ret;
        }
        break;
        case MAX7219_TEST_MODE: {
            // Send |MAX7219_TEST_ADDRESS|1| to all devices
            max7219_command_t command = { .address = MAX7219_TEST_ADDRESS, .data = 1 };
            return send_chain_command_private(handle, 0, command);
        }
        break;

        default:
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t led_driver_max7219_set_mode(led_driver_max7219_handle_t handle, uint8_t chainId, max7219_mode_t mode) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");
    ESP_RETURN_ON_ERROR(check_max_chain_id_private(handle, chainId), LedDriverMax7219LogTag, "Invalid chain ID");

    switch (mode) {
        case MAX7219_SHUTDOWN_MODE:
        case MAX7219_NORMAL_MODE: {
            ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_TIMEOUT, LedDriverMax7219LogTag, "Could not acquire mutex");

            // Take exclusive access of the SPI bus
            esp_err_t ret = ESP_OK;
            ESP_GOTO_ON_ERROR(spi_device_acquire_bus(handle->spi_device_handle, portMAX_DELAY), cleanup, LedDriverMax7219LogTag, "Unable to acquire SPI bus");

                max7219_command_t* buffer = get_command_buffer_private(handle);

                // Leave test mode (if on) by sending |MAX7219_TEST_ADDRESS|0| to the requested device
                {
                    max7219_command_t command = { .address = MAX7219_TEST_ADDRESS, .data = 0 };
                    for (uint8_t deviceIndex = 0; deviceIndex < handle->hw_config.chain_length; deviceIndex++) {
                        buffer[deviceIndex] = command;
                    }
                    ESP_GOTO_ON_ERROR(led_driver_max7219_send_private(handle, buffer, handle->hw_config.chain_length), releasebus, LedDriverMax7219LogTag, "Failed to send commands to chain");
                }

                // Enter normal or shutdown mode by sending |MAX7219_SHUTDOWN_ADDRESS|<0 or 1>| to the requested device
                {
                    max7219_command_t command = { .address = MAX7219_SHUTDOWN_ADDRESS, .data = mode == MAX7219_SHUTDOWN_MODE ? 0 : 1 };
                    for (uint8_t deviceIndex = 0; deviceIndex < handle->hw_config.chain_length; deviceIndex++) {
                        buffer[deviceIndex] = command;
                    }
                    ESP_GOTO_ON_ERROR(led_driver_max7219_send_private(handle, buffer, handle->hw_config.chain_length), releasebus, LedDriverMax7219LogTag, "Failed to send commands to chain");
                }

releasebus:
            // Release access to the SPI bus
            spi_device_release_bus(handle->spi_device_handle);

cleanup:
            // Release mutex
            if (xSemaphoreGive(handle->mutex) != pdTRUE) {
                ESP_LOGE(LedDriverMax7219LogTag, "Could not release mutex - Exiting without releasing mutex which may cause a deadlock later");
            }

            return ret;
        }
        break;

        case MAX7219_TEST_MODE: {
            // Send |MAX7219_TEST_ADDRESS|1| to the requested device
            max7219_command_t command = { .address = MAX7219_TEST_ADDRESS, .data = 1 };
            return send_chain_command_private(handle, chainId, command);
        }
        break;

        default:
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t led_driver_max7219_set_chain_intensity(led_driver_max7219_handle_t handle, max7219_intensity_t intensity) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");

    // Send |MAX7219_INTENSITY_ADDRESS|<intensity>| to all devices
    max7219_command_t command = { .address = MAX7219_INTENSITY_ADDRESS, .data = intensity };
    return send_chain_command_private(handle, 0, command);
}

esp_err_t led_driver_max7219_set_intensity(led_driver_max7219_handle_t handle, uint8_t chainId, max7219_intensity_t intensity) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");
    ESP_RETURN_ON_ERROR(check_max_chain_id_private(handle, chainId), LedDriverMax7219LogTag, "Invalid chain ID");

    // Send |MAX7219_INTENSITY_ADDRESS|<intensity>| to the requested device
    max7219_command_t command = { .address = MAX7219_INTENSITY_ADDRESS, .data = intensity };
    return send_chain_command_private(handle, chainId, command);
}

esp_err_t led_driver_max7219_set_digit(led_driver_max7219_handle_t handle, uint8_t chainId, uint8_t digit, uint8_t digitCode) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");
    ESP_RETURN_ON_ERROR(check_max_chain_id_private(handle, chainId), LedDriverMax7219LogTag, "Invalid chain ID");
    ESP_RETURN_ON_ERROR(check_max_digit_private(handle, digit), LedDriverMax7219LogTag, "Invalid digit");

    // Send |MAX7219_DIGIT<digit>_ADDRESS|<digitCode>| to the requested device
    max7219_command_t command = { .address = digit, .data = digitCode };
    return send_chain_command_private(handle, chainId, command);
}


esp_err_t led_driver_max7219_set_digits(led_driver_max7219_handle_t handle, uint8_t startChainId, uint8_t startDigitId, const uint8_t digitCodes[], uint8_t digitCodesCount) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");
    ESP_RETURN_ON_ERROR(check_max_chain_id_private(handle, startChainId), LedDriverMax7219LogTag, "Invalid chain ID");
    ESP_RETURN_ON_ERROR(check_max_digit_private(handle, startDigitId), LedDriverMax7219LogTag, "Invalid start digit");
    ESP_RETURN_ON_ERROR(check_bulk_symbols_array_length(handle, startChainId, startDigitId, digitCodesCount), LedDriverMax7219LogTag, "Invalid number of digit codes provided");

    ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_TIMEOUT, LedDriverMax7219LogTag, "Could not acquire mutex");

    max7219_command_t* buffer = get_command_buffer_private(handle);
    uint8_t dstDigitIndex = startDigitId;
    uint8_t deviceIndex = handle->hw_config.chain_length - startChainId;

    // Take exclusive access of the SPI bus
    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(spi_device_acquire_bus(handle->spi_device_handle, portMAX_DELAY), cleanup, LedDriverMax7219LogTag, "Unable to acquire SPI bus");

        for (uint8_t srcDigitIndex = 0; srcDigitIndex < digitCodesCount; srcDigitIndex++) {
            // Send |MAX7219_DIGIT<digit>_ADDRESS|<digitCode>| to the correct device in the chain 
            memset(buffer, MAX7219_NOOP_ADDRESS, handle->hw_config.chain_length * sizeof(max7219_command_t));
            max7219_command_t command = { .address = dstDigitIndex, .data = digitCodes[srcDigitIndex] };
            buffer[deviceIndex] = command;

            ESP_GOTO_ON_ERROR(led_driver_max7219_send_private(handle, buffer, handle->hw_config.chain_length), releasebus, LedDriverMax7219LogTag, "Failed to send commands to chain");

            dstDigitIndex++;
            if (dstDigitIndex > MAX7219_MAX_DIGIT) {
                dstDigitIndex = MAX7219_MIN_DIGIT;
                deviceIndex--;
            }
        }

releasebus:
    // Release access to the SPI bus
    spi_device_release_bus(handle->spi_device_handle);

cleanup:
    // Release mutex
    if (xSemaphoreGive(handle->mutex) != pdTRUE) {
        ESP_LOGE(LedDriverMax7219LogTag, "Could not release mutex - Exiting without releasing mutex which may cause a deadlock later");
    }

    return ret;
}


esp_err_t led_driver_max7219_set_chain(led_driver_max7219_handle_t handle, uint8_t digitCode) {
    ESP_RETURN_ON_ERROR(check_max_handle_private(handle), LedDriverMax7219LogTag, "Invalid handle");

    ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_TIMEOUT, LedDriverMax7219LogTag, "Could not acquire mutex");

    // Take exclusive access of the SPI bus
    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(spi_device_acquire_bus(handle->spi_device_handle, portMAX_DELAY), cleanup, LedDriverMax7219LogTag, "Unable to acquire SPI bus");

        // Send |MAX7219_DIGIT<digit>_ADDRESS|<digitCode>| to all devices
        // NOTE: We first clear digit 1 on all devices, then digit 2 on all devices and so on
        max7219_command_t* buffer = get_command_buffer_private(handle);
        for (uint8_t digit = MAX7219_MIN_DIGIT; digit <= MAX7219_MAX_DIGIT; digit++) {
            max7219_command_t command = { .address = digit, .data = digitCode };
            for (uint8_t deviceIndex = 0; deviceIndex < handle->hw_config.chain_length; deviceIndex++) {
                buffer[deviceIndex] = command;
            }
            ESP_GOTO_ON_ERROR(led_driver_max7219_send_private(handle, buffer, handle->hw_config.chain_length), releasebus, LedDriverMax7219LogTag, "Failed to send commands to chain");
        }

releasebus:
    // Release access to the SPI bus
    spi_device_release_bus(handle->spi_device_handle);

cleanup:
    // Release mutex
    if (xSemaphoreGive(handle->mutex) != pdTRUE) {
        ESP_LOGE(LedDriverMax7219LogTag, "Could not release mutex - Exiting without releasing mutex which may cause a deadlock later");
    }

    return ret;
}



static inline __attribute__((always_inline)) max7219_command_t* get_command_buffer_private(led_driver_max7219_handle_t handle) {
    return handle->commands.use_inline_buffer ? handle->commands.commands_data : handle->commands.commands_buffer;
}

static esp_err_t send_chain_command_private(led_driver_max7219_handle_t handle, uint8_t chainId, const max7219_command_t cmd) {
    ESP_RETURN_ON_FALSE(xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_TIMEOUT, LedDriverMax7219LogTag, "Could not acquire mutex");

    // Take exclusive access of the SPI bus
    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(spi_device_acquire_bus(handle->spi_device_handle, portMAX_DELAY), cleanup, LedDriverMax7219LogTag, "Unable to acquire SPI bus");

        max7219_command_t* buffer = get_command_buffer_private(handle);
        if (chainId == 0) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
            ESP_LOGI(LedDriverMax7219LogTag, "Sending { address: 0x%02X, data: 0x%02X } to all devices", cmd.address, cmd.data);
#endif
            // Send all devices the same .address and .data
            for (uint8_t deviceIndex = 0; deviceIndex < handle->hw_config.chain_length; deviceIndex++) {
                buffer[deviceIndex] = cmd;
            }
        } else {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
            ESP_LOGI(LedDriverMax7219LogTag, "Sending { address: 0x%02X, data: 0x%02X } to device %d", cmd.address, cmd.data, chainId);
#endif
            // Target a specific device in the chain - The device is given in chainId which is 1-based
            // The array is initialized to 0 which means .address is already set to MAX7219_NOOP_ADDRESS and .data is already set to 0
            // The data for the last device on the chain needs to be sent first so deviceId n is at index hw_config.chain_length - 1 in the array
            uint8_t deviceIndex = handle->hw_config.chain_length - chainId;
            memset(buffer, MAX7219_NOOP_ADDRESS, handle->hw_config.chain_length * sizeof(max7219_command_t));
            buffer[deviceIndex] = cmd;
        }

        ESP_GOTO_ON_ERROR(led_driver_max7219_send_private(handle, buffer, handle->hw_config.chain_length), releasebus, LedDriverMax7219LogTag, "Failed to send commands to chain");

releasebus:
    // Release access to the SPI bus
    spi_device_release_bus(handle->spi_device_handle);

cleanup:
    // Release mutex
    if (xSemaphoreGive(handle->mutex) != pdTRUE) {
        ESP_LOGE(LedDriverMax7219LogTag, "Could not release mutex - Exiting without releasing mutex which may cause a deadlock later");
    }

    return ret;
}

static esp_err_t led_driver_max7219_send_private(led_driver_max7219_handle_t handle, const max7219_command_t* const data, uint16_t commandsCount) {
    uint16_t lengthInBytes = sizeof(max7219_command_t) * commandsCount;
    bool useTxData = lengthInBytes <= 4;
    spi_transaction_t spiTransaction = {
        .flags = useTxData ? SPI_TRANS_USE_TXDATA : 0,
        .length = lengthInBytes * 8,
        .rxlength = 0
    };

    if (useTxData) {
        memcpy(spiTransaction.tx_data, data, lengthInBytes);
    } else {
        spiTransaction.tx_buffer = data;
    }

    return spi_device_transmit(handle->spi_device_handle, &spiTransaction);
}



static esp_err_t check_driver_configuration_private(const max7219_config_t* config) {
    if (config == NULL) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "Configuration must not be NULL");
#endif
        return ESP_ERR_INVALID_ARG;
    }

    // Check SPI configuration - Clock speed must be non zero and up to 10 MHz
    if ((config->spi_cfg.clock_speed_hz <= 0) || (config->spi_cfg.clock_speed_hz > 10 * 1000000)) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "spi_cfg.clock_speed_hz must be > 0 and <= 10 MHz");
#endif
        return ESP_ERR_INVALID_ARG;
    }

    // Check SPI configuration - /CS (LOAD) must be specified as it is used to latch data
    if (config->spi_cfg.spics_io_num == GPIO_NUM_NC) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "spi_cfg.spics_io_num must not be GPIO_NUM_NC");
#endif
        return ESP_ERR_INVALID_ARG;
    }

    // Check hardware configuration - Chain length must be at least 1 and less than 255
    if ((config->hw_config.chain_length < 1) || (config->hw_config.chain_length > 254)) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "hw_config.chain_length must be >= 1 and <= 254");
#endif
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

static esp_err_t check_max_handle_private(led_driver_max7219_handle_t handle) {
    if (handle == NULL) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "handle must not be NULL");
#endif
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->spi_device_handle == NULL) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "led_driver_max7219_init() must be called before any other function");
#endif
        return ESP_ERR_INVALID_STATE;
    }

    if (handle->mutex == NULL) {
#if CONFIG_MAX_7219_7221_ENABLE_DEBUG_LOG
        ESP_LOGE(LedDriverMax7219LogTag, "led_driver_max7219_init() must be called before any other function");
#endif
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

static esp_err_t check_max_chain_id_private(led_driver_max7219_handle_t handle, uint8_t chainId) {
    return (chainId >= 1) && (chainId <= handle->hw_config.chain_length) ? ESP_OK : ESP_ERR_INVALID_ARG;
}

static esp_err_t check_max_digit_private(led_driver_max7219_handle_t handle, uint8_t digit) {
    return (digit >= MAX7219_MIN_DIGIT) && (digit <= MAX7219_MAX_DIGIT) ? ESP_OK : ESP_ERR_INVALID_ARG;
}

static esp_err_t check_bulk_symbols_array_length(led_driver_max7219_handle_t handle, uint8_t startChainId, uint8_t startDigitId, uint8_t digitCodesCount) {
    // Number of remaining digits starting at device 'startChainId' and at digit 'startDigitId'
    const uint8_t availableDigits = ((handle->hw_config.chain_length - startChainId) * MAX7219_MAX_DIGIT) + (MAX7219_MAX_DIGIT - startDigitId) + 1;
    return (digitCodesCount > 0) && (digitCodesCount <= availableDigits) ? ESP_OK : ESP_ERR_INVALID_ARG;
}