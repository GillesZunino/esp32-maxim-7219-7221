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
 * @brief MAXIM 7219 / 7221 Code B font symbols.
 */
typedef enum {
    MAXIM7219_CODE_B_FONT_0 = 0,        ///< Digit 0
    MAXIM7219_CODE_B_FONT_1 = 1,        ///< Digit 1
    MAXIM7219_CODE_B_FONT_2 = 2,        ///< Digit 2
    MAXIM7219_CODE_B_FONT_3 = 3,        ///< Digit 3
    MAXIM7219_CODE_B_FONT_4 = 4,        ///< Digit 4
    MAXIM7219_CODE_B_FONT_5 = 5,        ///< Digit 5
    MAXIM7219_CODE_B_FONT_6 = 6,        ///< Digit 6
    MAXIM7219_CODE_B_FONT_7 = 7,        ///< Digit 7
    MAXIM7219_CODE_B_FONT_8 = 8,        ///< Digit 8
    MAXIM7219_CODE_B_FONT_9 = 9,        ///< Digit 9
    MAXIM7219_CODE_B_FONT_MINUS = 10,   ///< Minus sign
    MAXIM7219_CODE_B_FONT_E = 11,       ///< Letter E
    MAXIM7219_CODE_B_FONT_H = 12,       ///< Letter H
    MAXIM7219_CODE_B_FONT_L = 13,       ///< Letter L
    MAXIM7219_CODE_B_FONT_P = 14,       ///< Letter P
    MAXIM7219_CODE_B_FONT_BLANK = 15,   ///< Blank

    MAXIM7219_CODE_B_DP_MASK = 0x80     ///< Decimal Point mask - Cmbine with other symbols to add a decimal point as `MAXIM7219_CODE_B_FONT_0 | MAXIM7219_CODE_B_DP_MASK`
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
    MAXIM7219_SEGMENT_G = 0x01,         ///< Segment G
    MAXIM7219_SEGMENT_F = 0x02,         ///< Segment F
    MAXIM7219_SEGMENT_E = 0x04,         ///< Segment E
    MAXIM7219_SEGMENT_D = 0x08,         ///< Segment D
    MAXIM7219_SEGMENT_C = 0x10,         ///< Segment C
    MAXIM7219_SEGMENT_B = 0x20,         ///< Segment B
    MAXIM7219_SEGMENT_A = 0x40,         ///< Segment A
    MAXIM7219_SEGMENT_DP = 0x80         ///< Segment Decimal Point
} maxim7219_segment_t;

/**
 * @brief Custom 7 Segment Display symbols.
 */
typedef enum {
    MAXIM7219_CUSTOM_0 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_F,                         ///< Digit 0
    MAXIM7219_CUSTOM_1 = MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C,                                                                                                                 ///< Digit 1
    MAXIM7219_CUSTOM_2 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D,                                               ///< Digit 2
    MAXIM7219_CUSTOM_3 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D,                                               ///< Digit 3
    MAXIM7219_CUSTOM_4 = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C,                                                                                           ///< Digit 4
    MAXIM7219_CUSTOM_5 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D,                                               ///< Digit 5
    MAXIM7219_CUSTOM_6 = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_E,                                               ///< Digit 6
    MAXIM7219_CUSTOM_7 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_G,                                                                     ///< Digit 7
    MAXIM7219_CUSTOM_8 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G,   ///< Digit 8
    MAXIM7219_CUSTOM_9 = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G,                         ///< Digit 9
    MAXIM7219_CUSTOM_A = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_G,                         ///< Letter A
    MAXIM7219_CUSTOM_C = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D,                                                                     ///< Letter C
    MAXIM7219_CUSTOM_E = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_G,                                               ///< Letter E
    MAXIM7219_CUSTOM_F = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_G,                                                                     ///< Letter F
    MAXIM7219_CUSTOM_H = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C,                                               ///< Letter H
    MAXIM7219_CUSTOM_J = MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_D,                                                                                           ///< Letter J
    MAXIM7219_CUSTOM_L = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D,                                                                                           ///< Letter L
    MAXIM7219_CUSTOM_P = MAXIM7219_SEGMENT_A | MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_B | MAXIM7219_SEGMENT_G | MAXIM7219_SEGMENT_E,                                               ///< Letter P
    MAXIM7219_CUSTOM_U = MAXIM7219_SEGMENT_F | MAXIM7219_SEGMENT_E | MAXIM7219_SEGMENT_D | MAXIM7219_SEGMENT_C | MAXIM7219_SEGMENT_B,                                               ///< Letter U
    MAXIM7219_CUSTOM_MINUS = MAXIM7219_SEGMENT_G,                                                                                                                                   ///< Minus sign
    MAXIM7219_CUSTOM_BLANK = 0                                                                                                                                                      ///< Blank
} maxim7219_custom_font_t;

/**
 * @brief MAXIM 7219 / 7221 code B digit decode.
 */
typedef enum {
    MAXIM7219_CODE_B_NO_DECODE = 0x00,          ///< Code B font decode disabled

    MAXIM7219_CODE_B_DECODE_DIGIT_1 = 0x01,     ///< Code B font decode for the first digit
    MAXIM7219_CODE_B_DECODE_DIGIT_2 = 0x02,     ///< Code B font decode for the second digit
    MAXIM7219_CODE_B_DECODE_DIGIT_3 = 0x04,     ///< Code B font decode for the third digit
    MAXIM7219_CODE_B_DECODE_DIGIT_4 = 0x08,     ///< Code B font decode for the fourth digit
    MAXIM7219_CODE_B_DECODE_DIGIT_5 = 0x10,     ///< Code B font decode for the fitfh digit
    MAXIM7219_CODE_B_DECODE_DIGIT_6 = 0x20,     ///< Code B font decode for the sixth digit
    MAXIM7219_CODE_B_DECODE_DIGIT_7 = 0x40,     ///< Code B font decode for the seventh digit
    MAXIM7219_CODE_B_DECODE_DIGIT_8 = 0x40,     ///< Code B font decode for the eighth digit

    MAXIM7219_CODE_B_DECODE_ALL = MAXIM7219_CODE_B_DECODE_DIGIT_1 | MAXIM7219_CODE_B_DECODE_DIGIT_2 | MAXIM7219_CODE_B_DECODE_DIGIT_3 |
                                  MAXIM7219_CODE_B_DECODE_DIGIT_4 | MAXIM7219_CODE_B_DECODE_DIGIT_5 | MAXIM7219_CODE_B_DECODE_DIGIT_6 |
                                  MAXIM7219_CODE_B_DECODE_DIGIT_7 | MAXIM7219_CODE_B_DECODE_DIGIT_8                                         ///< Code B font decode for all digits
} maxim7219_decode_mode_t;

/**
 * @brief MAXIM 7219 / 7221 operation mode.
 */
typedef enum {
    MAXIM7219_SHUTDOWN_MODE = 0,        ///< Shutdown mode. All digits are blanked
    MAXIM7219_NORMAL_MODE = 1,          ///< Normal mode. All digits are displayed normally
    MAXIM7219_TEST_MODE = 2             ///< Test mode. All segments are turned on, intensity settings are ignored
} maxim7219_mode_t;

/**
 * @brief MAXIM 7219 / 7221 intensity PWM step.
 * @note The intensity is controlled by a PWM signal. The duty cycle of the PWM signal depends on the type of device:
 *       * MAXIM 7219 MAXIM7219_INTENSITY_STEP_1 means 1/16 duty cycle.
 *       * MAXIM 7221 MAXIM7219_INTENSITY_STEP_1 means 1/32 duty cycle.
 */
typedef enum {
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_1 = 0x00,       ///< Intensity duty cycle 1/16 (MAXIM 7219) or 1/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_2 = 0x01,       ///< Intensity duty cycle 2/16 (MAXIM 7219) or 3/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_3 = 0x02,       ///< Intensity duty cycle 3/16 (MAXIM 7219) or 5/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_4 = 0x03,       ///< Intensity duty cycle 4/16 (MAXIM 7219) or 7/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_5 = 0x04,       ///< Intensity duty cycle 5/16 (MAXIM 7219) or 9/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_6 = 0x05,       ///< Intensity duty cycle 6/16 (MAXIM 7219) or 11/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_7 = 0x06,       ///< Intensity duty cycle 7/16 (MAXIM 7219) or 13/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_8 = 0x07,       ///< Intensity duty cycle 8/16 (MAXIM 7219) or 15/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_9 = 0x08,       ///< Intensity duty cycle 9/16 (MAXIM 7219) or 17/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_10 = 0x09,      ///< Intensity duty cycle 10/16 (MAXIM 7219) or 19/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_11 = 0x0A,      ///< Intensity duty cycle 11/16 (MAXIM 7219) or 21/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_12 = 0x0B,      ///< Intensity duty cycle 12/16 (MAXIM 7219) or 23/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_13 = 0x0C,      ///< Intensity duty cycle 13/16 (MAXIM 7219) or 25/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_14 = 0x0D,      ///< Intensity duty cycle 14/16 (MAXIM 7219) or 27/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_15 = 0x0E,      ///< Intensity duty cycle 15/16 (MAXIM 7219) or 29/32 (MAXIM 7221)
    MAXIM7219_INTENSITY_DUTY_CYCLE_STEP_16 = 0x0F       ///< Intensity duty cycle 16/16 (MAXIM 7219) or 31/32 (MAXIM 7221)
} maxim7219_intensity_t;



/**
 * @brief Configuration of the SPI bus for MAXIM 7219 / 7221 device.
 */
typedef struct maxim7219_spi_config {
    spi_host_device_t host_id;          ///< SPI bus ID. Which buses are available depends on the specific device
    spi_clock_source_t clock_source;    ///< Select SPI clock source, `SPI_CLK_SRC_DEFAULT` by default
    int clock_speed_hz;                 ///< SPI clock speed in Hz. Derived from `clock_source`
    int input_delay_ns;                 ///< Maximum data valid time of slave. The time required between SCLK and MISO
    int spics_io_num;                   ///< CS GPIO pin for this device, or `GPIO_NUM_NC` (-1) if not used
    int queue_size;                     ///< SPI transaction queue size. See 'spi_device_queue_trans()'
} maxim7219_spi_config_t;

/**
 * @brief Type of MAXIM LED driver device connected.
 */
typedef enum {
    MAXIM_7219_TYPE = 1,                ///< MAXIM 7219 LED Driver variant
    MAXIM_7221_TYPE = 2                 ///< MAXIM 7221 LED Driver variant
} maxim7219_type_t;

/**
 * @brief MAXIM LED Driver hardware configuration.
 */
typedef struct maxim7219_hw_config {
    uint8_t chain_length;               ///< Number of MAXIM 7219 / 7221 connected (1 to 255). See "Cascading Drivers" in the MAXIM datasheet
    maxim7219_type_t device_type;       ///< Type of device. These devices are mostly compatible but some operations (i.e. brightness setting) are different 
} maxim7219_hw_config_t;

/**
 * @brief Configuration of MAXIM 7219 / 7221 device.
 */
typedef struct maxim7219_config {
    maxim7219_spi_config_t spi_cfg;       ///< SPI configuration for MCP2515
    maxim7219_hw_config_t hw_config;      ///< MAXIM 7219 / 7221 hardware configuration
} maxim7219_config_t;



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
 * @brief Configure digit decoding on all MAXIM 7219 / 7221 devices on the chain.
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  decodeMode The decode mode to configure. See `maxim7219_decode_mode_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_chain_decode(led_driver_maxim7219_handle_t handle, maxim7219_decode_mode_t decodeMode);

/**
 * @brief Configure digit decoding on a specific MAXIM 7219 / 7221 device on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  chainId Index of the MAXIM device to configure starting at 1 for the first device
 * @param[in]  decodeMode The decode mode to configure. See `maxim7219_decode_mode_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_decode(led_driver_maxim7219_handle_t handle, uint8_t chainId, maxim7219_decode_mode_t decodeMode);

/**
 * @brief Configure scan limits all MAXIM 7219 / 7221 devices on the chain.
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  digits The number of digits to limit scan to. Must be between 1 and 8
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_chain_scan_limit(led_driver_maxim7219_handle_t handle, uint8_t digits);

/**
 * @brief Configure scan limit on a specific MAXIM 7219 / 7221 device on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  chainId Index of the MAXIM device to configure starting at 1 for the first device
 * @param[in]  digits The number of digits to limit scan to. Must be between 1 and 8
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_scan_limit(led_driver_maxim7219_handle_t handle, uint8_t chainId, uint8_t digits);



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
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  chainId Index of the MAXIM device to configure starting at 1 for the first device
 * @param[in]  mode The mode to configure. See `maxim7219_mode_t` for possible values 
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_mode(led_driver_maxim7219_handle_t handle, uint8_t chainId, maxim7219_mode_t mode);


/**
 * @brief Configure intensity on all MAXIM 7219 / 7221 devices on the chain.
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  intensity The duty cycle to set. See `maxim7219_intensity_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_chain_intensity(led_driver_maxim7219_handle_t handle, maxim7219_intensity_t intensity);

/**
 * @brief Set intensity on a specific MAXIM 7219 / 7221 devices on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  chainId Index of the MAXIM device to configure starting at 1 for the first device
 * @param[in]  intensity The duty cycle to set. See `maxim7219_intensity_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_intensity(led_driver_maxim7219_handle_t handle, uint8_t chainId, maxim7219_intensity_t intensity);

/**
 * @brief Set the given digit code on a MAXIM 7219 / 7221 device on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAXIM 7219 / 7221 driver
 * @param[in]  chainId Index of the MAXIM device to configure starting at 1 for the first device
 * @param[in]  digit The digit to set (1 to 8)
 * @param[in]  digitCode The digit code to set. A `maxim7219_code_b_font_t` value for digits in Code B decode mode or a combination of `maxim7219_segment_t` values for devices in no decode mode
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_digit(led_driver_maxim7219_handle_t handle, uint8_t chainId, uint8_t digit, uint8_t digitCode);



#ifdef __cplusplus
}
#endif