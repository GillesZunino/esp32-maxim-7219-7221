// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#pragma once


#include <esp_err.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Handle to a MAXIM 7219 / 7221 device.
 */
struct led_driver_maxim7219;
typedef struct led_driver_maxim7219* led_driver_maxim7219_handle_t; ///< Handle to a MAXIM 7219 / 7221 device

/**
 * @brief MAXIM 7219 / 7221 register addresses.
 */
typedef enum {
    MAXIM7219_NOOP_ADDRESS = 0x00,
    MAXIM7219_DIGIT0_ADDRESS = 0x01,
    MAXIM7219_DIGIT1_ADDRESS = 0x02,
    MAXIM7219_DIGIT2_ADDRESS = 0x03,
    MAXIM7219_DIGIT3_ADDRESS = 0x04,
    MAXIM7219_DIGIT4_ADDRESS = 0x05,
    MAXIM7219_DIGIT5_ADDRESS = 0x06,
    MAXIM7219_DIGIT6_ADDRESS = 0x07,
    MAXIM7219_DIGIT7_ADDRESS = 0x08,
    MAXIM7219_DECODE_MODE_ADDRESS = 0x09,
    MAXIM7219_INTENSITY_ADDRESS = 0x0A,
    MAXIM7219_SCAN_LIMIT_ADDRESS = 0x0B,
    MAXIM7219_SHUTDOWN_ADDRESS = 0x0C,
    MAXIM7219_TEST_ADDRESS = 0x0F
} maxim7219_address_t;

/**
 * @brief MAXIM 7219 / 7221 Code B font symbols.
 */
typedef enum {
    MAXIM7219_CODE_B_FONT_0 = 0,
    MAXIM7219_CODE_B_FONT_1 = 1,
    MAXIM7219_CODE_B_FONT_2 = 2,
    MAXIM7219_CODE_B_FONT_3 = 3,
    MAXIM7219_CODE_B_FONT_4 = 4,
    MAXIM7219_CODE_B_FONT_5 = 5,
    MAXIM7219_CODE_B_FONT_6 = 6,
    MAXIM7219_CODE_B_FONT_7 = 7,
    MAXIM7219_CODE_B_FONT_8 = 8,
    MAXIM7219_CODE_B_FONT_9 = 9,
    MAXIM7219_CODE_B_FONT_MINUS = 10,
    MAXIM7219_CODE_B_FONT_E = 11,
    MAXIM7219_CODE_B_FONT_H = 12,
    MAXIM7219_CODE_B_FONT_L = 13,
    MAXIM7219_CODE_B_FONT_P = 14,
    MAXIM7219_CODE_B_FONT_BLANK = 15
} maxim7219_code_b_font_t;

/**
 * @brief MAXIM 7219 / 7221 segments.
 * @note - A -
         |   |
         F   B
         |   |
         - G -
         |   |
         E   C
         |   |
         - D - DP
 * 
 */
typedef enum {
    MAXIM7219_SEGMENT_G = 0x01,
    MAXIM7219_SEGMENT_F = 0x02,
    MAXIM7219_SEGMENT_E = 0x04,
    MAXIM7219_SEGMENT_D = 0x08,
    MAXIM7219_SEGMENT_C = 0x10,
    MAXIM7219_SEGMENT_B = 0x20,
    MAXIM7219_SEGMENT_A = 0x40,
    MAXIM7219_SEGMENT_DP = 0x80
} maxim7219_segment_t;

/**
 * @brief Custom 7 Segment Display symbols.
 */
typedef enum {
    MAXIM7219_CUSTOM_0 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_F,
    MAXIM7219_CUSTOM_1 = MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C,
    MAXIM7219_CUSTOM_2 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D,
    MAXIM7219_CUSTOM_3 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D,
    MAXIM7219_CUSTOM_4 = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C,
    MAXIM7219_CUSTOM_5 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D,
    MAXIM7219_CUSTOM_6 = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_E,
    MAXIM7219_CUSTOM_7 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_G,
    MAXIM7219_CUSTOM_8 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G,
    MAXIM7219_CUSTOM_9 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G,
    MAXIM7219_CUSTOM_A = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G,
    MAXIM7219_CUSTOM_C = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D,
    MAXIM7219_CUSTOM_E = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_G,
    MAXIM7219_CUSTOM_F = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_G,
    MAXIM7219_CUSTOM_H = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C,
    MAXIM7219_CUSTOM_J = MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D,
    MAXIM7219_CUSTOM_L = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D,
    MAXIM7219_CUSTOM_P = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_E,
    MAXIM7219_CUSTOM_U = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_B,
    MAXIM7219_CUSTOM_DASH = MAXIM7219_SEGMENT_G,
    MAXIM7219_CUSTOM_BLANK = 0
} maxim7219_extended_font_t;


/**
 * @brief Configuration of the SPI bus for MAXIM 7219 / 7221 device.
 */
typedef struct maxim7219_spi_config {
    spi_host_device_t host_id;          ///< SPI bus ID. Which buses are available depends on the specific chip
    spi_clock_source_t clock_source;    ///< Select SPI clock source, `SPI_CLK_SRC_DEFAULT` by default
    int clock_speed_hz;                 ///< SPI clock speed in Hz. Derived from `clock_source`
    int input_delay_ns;                 ///< Maximum data valid time of slave. The time required between SCLK and MISO
    int spics_io_num;                   ///< CS GPIO pin for this device, or `GPIO_NUM_NC` (-1) if not used
    int queue_size;                     ///< SPI transaction queue size. See 'spi_device_queue_trans()'
} maxim7219_spi_config_t;

/**
 * @brief Type of MAXIM LED driver chip connected.
 */
typedef enum {
    MAXIM_7219_TYPE = 1,                ///< MAXIM 7219 LED Driver variant
    MAXIM_7221_TYPE = 2                 ///< MAXIM 7221 LED Driver variant
} maxim7219_type_t;

/**
 * @brief MAXIM LED Driver hardware configuration.
 */
typedef struct maxim7219_hw_config {
    uint8_t scan_limit;                 ///< MAXIM 7219 / 7221 scan limit (1 to 8)
    uint8_t chain_length;               ///< Number of MAXIM 7219 / 7221 connected (1 to 255). See "Cascading Drivers" in the MAXIM datasheet
    maxim7219_type_t chip_type;         ///< Type of chip. These chips are jostyl compatible but some operations (i.e. brightness setting) is different 
} maxim7219_hw_config_t;

/**
 * @brief Configuration of MAXIM 7219 / 7221 device.
 */
typedef struct maxim7219_config {
    maxim7219_spi_config_t spi_cfg;       ///< SPI configuration for MCP2515
    maxim7219_hw_config_t hw_config;      ///< MAXIM 7219 / 7221 hardware configuration
} maxim7219_config_t;

/**
 * @brief MAXIM 7219 / 7221 operation mode.
 */
typedef enum {
    MAXIM7219_SHUTDOWN_MODE = 0,
    MAXIM7219_NORMAL_MODE = 1,
    MAXIM7219_TEST_MODE = 2
} maxim7219_mode_t;


/**
 * @brief Initialize the MAXIM 7219 / 7221 driver.
 * 
 * @param[in]  config Pointer to a configuration structure for the MAXIM 7219 / 7221 driver
 * @param[out]  handle Pointer to a memory location which received the handle to the MAXIM 7219 / 7221 driver
 * 
 * @return
 *      - ESP_OK: Successfully installed driver
 *      - ESP_ERR_INVALID_ARG: Arguments are invalid, e.g. invalid clock source, ...
 *      - ESP_ERR_NO_MEM: Insufficient memory
 */
esp_err_t led_driver_max7219_init(const maxim7219_config_t* config, led_driver_maxim7219_handle_t* handle);

/**
 * @brief Free the MAXIM 7219 / 7221 driver.
 * 
 * @return
 *      - ESP_OK: Successfully uninstalled the driver
 *      - ESP_ERR_INVALID_STATE: Driver is not installed or in an invalid state
 */
esp_err_t led_driver_max7219_free(led_driver_maxim7219_handle_t handle);


/**
 * @brief Set the operation mode on all MAXIM 7219 / 7221 devices on the chain.
 *
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  mode The mode to configure. See `maxim7219_mode_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_chain_mode(led_driver_maxim7219_handle_t handle, maxim7219_mode_t mode);

/**
 * @brief Set the operation mode on a specific MAXIM 7219 / 7221 device on the chain.
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  chainId Index of the MAXIM chip to configure starting at 1 for the first device
 * @param[in]  mode The mode to configure. See `maxim7219_mode_t` for possible values 
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_mode(led_driver_maxim7219_handle_t handle, uint8_t chainId, maxim7219_mode_t mode);


#ifdef __cplusplus
}
#endif