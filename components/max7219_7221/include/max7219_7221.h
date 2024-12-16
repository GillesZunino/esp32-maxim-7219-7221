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


#define MAX7219_MIN_DIGIT 1   ///< A MAX7219 / MAX7221 can drive a minimum of 1 digit
#define MAX7219_MAX_DIGIT 8   ///< A MAX7219 / MAX7221 can drive a maximum of 8 digits


/**
 * @brief Handle to a MAX7219 / MAX7221 device.
 */
struct led_driver_max7219;
typedef struct led_driver_max7219* led_driver_max7219_handle_t; ///< Handle to a MAX7219 / MAX7221 device


/**
 * @brief MAX7219 / MAX7221 Code-B symbols.
 */
typedef enum {
    MAX7219_CODE_B_0 = 0,         ///< Digit 0
    MAX7219_CODE_B_1 = 1,         ///< Digit 1
    MAX7219_CODE_B_2 = 2,         ///< Digit 2
    MAX7219_CODE_B_3 = 3,         ///< Digit 3
    MAX7219_CODE_B_4 = 4,         ///< Digit 4
    MAX7219_CODE_B_5 = 5,         ///< Digit 5
    MAX7219_CODE_B_6 = 6,         ///< Digit 6
    MAX7219_CODE_B_7 = 7,         ///< Digit 7
    MAX7219_CODE_B_8 = 8,         ///< Digit 8
    MAX7219_CODE_B_9 = 9,         ///< Digit 9
    MAX7219_CODE_B_MINUS = 10,    ///< Minus sign
    MAX7219_CODE_B_E = 11,        ///< Letter E
    MAX7219_CODE_B_H = 12,        ///< Letter H
    MAX7219_CODE_B_L = 13,        ///< Letter L
    MAX7219_CODE_B_P = 14,        ///< Letter P
    MAX7219_CODE_B_BLANK = 15,    ///< Blank

    MAX7219_CODE_B_DP_MASK = 0x80 ///< Decimal Point mask - Combine with other symbols to add a decimal point as `MAX7219_CODE_B_0 | MAX7219_CODE_B_DP_MASK`
} max7219_code_b_font_t;

/**
 * @brief MAX7219 / MAX7221 segments.
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
    MAX7219_SEGMENT_G = 0x01,         ///< Segment G
    MAX7219_SEGMENT_F = 0x02,         ///< Segment F
    MAX7219_SEGMENT_E = 0x04,         ///< Segment E
    MAX7219_SEGMENT_D = 0x08,         ///< Segment D
    MAX7219_SEGMENT_C = 0x10,         ///< Segment C
    MAX7219_SEGMENT_B = 0x20,         ///< Segment B
    MAX7219_SEGMENT_A = 0x40,         ///< Segment A
    MAX7219_SEGMENT_DP = 0x80         ///< Segment Decimal Point
} max7219_segment_t;

/**
 * @brief Custom seven Segment Display symbols.
 */
typedef enum {
    MAX7219_DIRECT_ADDRESSING_0 = MAX7219_SEGMENT_A | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D | MAX7219_SEGMENT_E | MAX7219_SEGMENT_F,                       ///< Digit 0
    MAX7219_DIRECT_ADDRESSING_1 = MAX7219_SEGMENT_B | MAX7219_SEGMENT_C,                                                                                                       ///< Digit 1
    MAX7219_DIRECT_ADDRESSING_2 = MAX7219_SEGMENT_A | MAX7219_SEGMENT_B | MAX7219_SEGMENT_G | MAX7219_SEGMENT_E | MAX7219_SEGMENT_D,                                           ///< Digit 2
    MAX7219_DIRECT_ADDRESSING_3 = MAX7219_SEGMENT_A | MAX7219_SEGMENT_B | MAX7219_SEGMENT_G | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D,                                           ///< Digit 3
    MAX7219_DIRECT_ADDRESSING_4 = MAX7219_SEGMENT_F | MAX7219_SEGMENT_G | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C,                                                               ///< Digit 4
    MAX7219_DIRECT_ADDRESSING_5 = MAX7219_SEGMENT_A | MAX7219_SEGMENT_F | MAX7219_SEGMENT_G | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D,                                           ///< Digit 5
    MAX7219_DIRECT_ADDRESSING_6 = MAX7219_SEGMENT_F | MAX7219_SEGMENT_G | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D | MAX7219_SEGMENT_E,                                           ///< Digit 6
    MAX7219_DIRECT_ADDRESSING_7 = MAX7219_SEGMENT_A | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C,                                                                                   ///< Digit 7
    MAX7219_DIRECT_ADDRESSING_8 = MAX7219_SEGMENT_A | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D | MAX7219_SEGMENT_E | MAX7219_SEGMENT_F | MAX7219_SEGMENT_G,   ///< Digit 8
    MAX7219_DIRECT_ADDRESSING_9 = MAX7219_SEGMENT_A | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D | MAX7219_SEGMENT_F | MAX7219_SEGMENT_G,                       ///< Digit 9
    
    MAX7219_DIRECT_ADDRESSING_A = MAX7219_SEGMENT_A | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C | MAX7219_SEGMENT_E | MAX7219_SEGMENT_F | MAX7219_SEGMENT_G,                       ///< Letter A
    MAX7219_DIRECT_ADDRESSING_C = MAX7219_SEGMENT_A | MAX7219_SEGMENT_F | MAX7219_SEGMENT_E | MAX7219_SEGMENT_D,                                                               ///< Letter C
    MAX7219_DIRECT_ADDRESSING_E = MAX7219_SEGMENT_A | MAX7219_SEGMENT_F | MAX7219_SEGMENT_E | MAX7219_SEGMENT_D | MAX7219_SEGMENT_G,                                           ///< Letter E
    MAX7219_DIRECT_ADDRESSING_F = MAX7219_SEGMENT_A | MAX7219_SEGMENT_F | MAX7219_SEGMENT_E | MAX7219_SEGMENT_G,                                                               ///< Letter F
    MAX7219_DIRECT_ADDRESSING_H = MAX7219_SEGMENT_F | MAX7219_SEGMENT_E | MAX7219_SEGMENT_G | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C,                                           ///< Letter H
    MAX7219_DIRECT_ADDRESSING_J = MAX7219_SEGMENT_B | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D,                                                                                   ///< Letter J
    MAX7219_DIRECT_ADDRESSING_L = MAX7219_SEGMENT_F | MAX7219_SEGMENT_E | MAX7219_SEGMENT_D,                                                                                   ///< Letter L
    MAX7219_DIRECT_ADDRESSING_P = MAX7219_SEGMENT_A | MAX7219_SEGMENT_F | MAX7219_SEGMENT_B | MAX7219_SEGMENT_G | MAX7219_SEGMENT_E,                                           ///< Letter P
    MAX7219_DIRECT_ADDRESSING_U = MAX7219_SEGMENT_F | MAX7219_SEGMENT_E | MAX7219_SEGMENT_D | MAX7219_SEGMENT_C | MAX7219_SEGMENT_B,                                           ///< Letter U
    
    MAX7219_DIRECT_ADDRESSING_b = MAX7219_SEGMENT_F | MAX7219_SEGMENT_G | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D | MAX7219_SEGMENT_E,                                           ///< Letter b
    MAX7219_DIRECT_ADDRESSING_d = MAX7219_SEGMENT_B | MAX7219_SEGMENT_G | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D | MAX7219_SEGMENT_E,                                           ///< Letter d
    MAX7219_DIRECT_ADDRESSING_h = MAX7219_SEGMENT_F | MAX7219_SEGMENT_E | MAX7219_SEGMENT_G | MAX7219_SEGMENT_C,                                                               ///< Letter h
    MAX7219_DIRECT_ADDRESSING_o = MAX7219_SEGMENT_G | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D | MAX7219_SEGMENT_E,                                                               ///< Letter o
    MAX7219_DIRECT_ADDRESSING_r = MAX7219_SEGMENT_E | MAX7219_SEGMENT_G,                                                                                                       ///< Letter r
    MAX7219_DIRECT_ADDRESSING_t = MAX7219_SEGMENT_F | MAX7219_SEGMENT_G | MAX7219_SEGMENT_E | MAX7219_SEGMENT_D,                                                               ///< Letter t
    MAX7219_DIRECT_ADDRESSING_u = MAX7219_SEGMENT_E | MAX7219_SEGMENT_D | MAX7219_SEGMENT_C,                                                                                   ///< Letter u
    MAX7219_DIRECT_ADDRESSING_y = MAX7219_SEGMENT_F | MAX7219_SEGMENT_G | MAX7219_SEGMENT_B | MAX7219_SEGMENT_C | MAX7219_SEGMENT_D,                                           ///< Letter y 

    MAX7219_DIRECT_ADDRESSING_MINUS = MAX7219_SEGMENT_G,                                                                                                                       ///< Minus sign
    MAX7219_DIRECT_ADDRESSING_BLANK = 0                                                                                                                                        ///< Blank
} max7219_direct_addressing_font_t;

/**
 * @brief MAX7219 / MAX7221 Code-B digit decode.
 */
typedef enum {
    MAX7219_CODE_B_DECODE_NONE = 0x00,        ///< Code B font decode disabled

    MAX7219_CODE_B_DECODE_DIGIT_1 = 0x01,     ///< Code B font decode the first digit
    MAX7219_CODE_B_DECODE_DIGIT_2 = 0x02,     ///< Code B font decode the second digit
    MAX7219_CODE_B_DECODE_DIGIT_3 = 0x04,     ///< Code B font decode the third digit
    MAX7219_CODE_B_DECODE_DIGIT_4 = 0x08,     ///< Code B font decode the fourth digit
    MAX7219_CODE_B_DECODE_DIGIT_5 = 0x10,     ///< Code B font decode the fifth digit
    MAX7219_CODE_B_DECODE_DIGIT_6 = 0x20,     ///< Code B font decode the sixth digit
    MAX7219_CODE_B_DECODE_DIGIT_7 = 0x40,     ///< Code B font decode the seventh digit
    MAX7219_CODE_B_DECODE_DIGIT_8 = 0x80,     ///< Code B font decode the eighth digit

    MAX7219_CODE_B_DECODE_ALL = MAX7219_CODE_B_DECODE_DIGIT_1 | MAX7219_CODE_B_DECODE_DIGIT_2 | MAX7219_CODE_B_DECODE_DIGIT_3 |
                                MAX7219_CODE_B_DECODE_DIGIT_4 | MAX7219_CODE_B_DECODE_DIGIT_5 | MAX7219_CODE_B_DECODE_DIGIT_6 |
                                MAX7219_CODE_B_DECODE_DIGIT_7 | MAX7219_CODE_B_DECODE_DIGIT_8                                         ///< Code B font decode for all digits
} max7219_decode_mode_t;

/**
 * @brief MAX7219 / MAX7221 operation mode.
 */
typedef enum {
    MAX7219_SHUTDOWN_MODE = 0,        ///< Shutdown mode. All digits are blanked
    MAX7219_NORMAL_MODE = 1,          ///< Normal mode. All digits are displayed normally
    MAX7219_TEST_MODE = 2             ///< Test mode. All segments are turned on, intensity settings are ignored
} max7219_mode_t;

/**
 * @brief MAX7219 / MAX7221 intensity PWM step.
 * @note The intensity is controlled by a PWM signal. The duty cycle of the PWM signal depends on the type of device:
 *       * MAX7219 MAX7219_INTENSITY_STEP_1 means 1/16 duty cycle.
 *       * MAX7221 MAX7219_INTENSITY_STEP_1 means 1/32 duty cycle.
 */
typedef enum {
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_1 = 0x00,       ///< Intensity duty cycle 1/16 (MAX7219) or 1/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_2 = 0x01,       ///< Intensity duty cycle 2/16 (MAX7219) or 3/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_3 = 0x02,       ///< Intensity duty cycle 3/16 (MAX7219) or 5/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_4 = 0x03,       ///< Intensity duty cycle 4/16 (MAX7219) or 7/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_5 = 0x04,       ///< Intensity duty cycle 5/16 (MAX7219) or 9/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_6 = 0x05,       ///< Intensity duty cycle 6/16 (MAX7219) or 11/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_7 = 0x06,       ///< Intensity duty cycle 7/16 (MAX7219) or 13/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_8 = 0x07,       ///< Intensity duty cycle 8/16 (MAX7219) or 15/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_9 = 0x08,       ///< Intensity duty cycle 9/16 (MAX7219) or 17/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_10 = 0x09,      ///< Intensity duty cycle 10/16 (MAX7219) or 19/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_11 = 0x0A,      ///< Intensity duty cycle 11/16 (MAX7219) or 21/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_12 = 0x0B,      ///< Intensity duty cycle 12/16 (MAX7219) or 23/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_13 = 0x0C,      ///< Intensity duty cycle 13/16 (MAX7219) or 25/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_14 = 0x0D,      ///< Intensity duty cycle 14/16 (MAX7219) or 27/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_15 = 0x0E,      ///< Intensity duty cycle 15/16 (MAX7219) or 29/32 (MAX7221)
    MAX7219_INTENSITY_DUTY_CYCLE_STEP_16 = 0x0F       ///< Intensity duty cycle 16/16 (MAX7219) or 31/32 (MAX7221)
} max7219_intensity_t;



/**
 * @brief Configuration of the SPI bus for MAX7219 / MAX7221 device.
 */
typedef struct max7219_spi_config {
    spi_host_device_t host_id;          ///< SPI bus ID. Which buses are available depends on the specific device
    spi_clock_source_t clock_source;    ///< Select SPI clock source, `SPI_CLK_SRC_DEFAULT` by default
    int clock_speed_hz;                 ///< SPI clock speed in Hz. Derived from `clock_source`
    int input_delay_ns;                 ///< Maximum data valid time of slave. The time required between SCLK and MISO
    int spics_io_num;                   ///< CS GPIO pin for this device, or `GPIO_NUM_NC` (-1) if not used
    int queue_size;                     ///< SPI transaction queue size. See 'spi_device_queue_trans()'
} max7219_spi_config_t;

/**
 * @brief MAX7219 / MAX7221 LED Driver hardware configuration.
 */
typedef struct max7219_hw_config {
    uint8_t chain_length;               ///< Number of MAX7219 / MAX7221 connected (1 to 255). See "Cascading Drivers" in the  MAX7219 / MAX7221 datasheet
} max7219_hw_config_t;

/**
 * @brief Configuration of MAX7219 / MAX7221 device.
 */
typedef struct max7219_config {
    max7219_spi_config_t spi_cfg;       ///< SPI configuration for MAX7219 / MAX7221
    max7219_hw_config_t hw_config;      ///< MAX7219 / MAX7221 hardware configuration
} max7219_config_t;



/**
 * @brief Initialize the MAX7219 / MAX7221 driver.
 * 
 * @param[in]  config Pointer to a configuration structure for the MAX7219 / MAX7221 driver
 * @param[out] handle Pointer to a memory location which receives the handle to the MAX7219 / MAX7221 driver
 * 
 * @return
 *      - ESP_OK: Successfully installed driver
 *      - ESP_ERR_INVALID_ARG: Arguments are invalid, e.g. invalid clock source, ...
 *      - ESP_ERR_NO_MEM: Insufficient memory
 */
esp_err_t led_driver_max7219_init(const max7219_config_t* config, led_driver_max7219_handle_t* handle);

/**
 * @brief Free the MAX7219 / MAX7221 driver.
 * 
 * @param[in] handle Handle to the MAX7219 / MAX7221 driver
 * 
 * @return
 *      - ESP_OK: Successfully uninstalled the driver
 *      - ESP_ERR_INVALID_STATE: Driver is not installed or in an invalid state
 */
esp_err_t led_driver_max7219_free(led_driver_max7219_handle_t handle);



/**
 * @brief Configure digit decoding on all MAX7219 / MAX7221 devices on the chain.
 * 
 * @param[in] handle Handle to the MAX7219 / MAX7221 driver
 * @param[in] decodeMode The decode mode to configure. See `max7219_decode_mode_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_chain_decode(led_driver_max7219_handle_t handle, max7219_decode_mode_t decodeMode);

/**
 * @brief Configure digit decoding on a specific MAX7219 / MAX7221 device on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAX7219 / MAX7221 driver
 * @param[in]  chainId Index of the MAX7219 / MAX7221 device to configure starting at 1 for the first device
 * @param[in]  decodeMode The decode mode to configure. See `max7219_decode_mode_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_decode(led_driver_max7219_handle_t handle, uint8_t chainId, max7219_decode_mode_t decodeMode);

/**
 * @brief Configure scan limits on all MAX7219 / MAX7221 devices on the chain.
 * 
 * @param[in] handle Handle to the MAX7219 / MAX7221 driver
 * @param[in] digits The number of digits to limit scan to. Must be between 1 and 8
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_chain_scan_limit(led_driver_max7219_handle_t handle, uint8_t digits);

/**
 * @brief Configure scan limit on a specific MAX7219 / MAX7221 device on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAX7219 / MAX7221 driver
 * @param[in]  chainId Index of the MAX7219 / MAX7221 device to configure starting at 1 for the first device
 * @param[in]  digits The number of digits to limit scan to. Must be between 1 and 8
 *
 * @attention It is recommended to set the same scan limit on all devices otherwise a display might appear brighter than others.
 *            For example:
 *              * If twelve digits are required, set the scan limit to six on two MAX7219 / MAX7221 devices.
 *              * If eleven digits are required, set the scan limit to six on two MAX7219 / MAX7221 devices and leave one digit driver unconnected.
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_configure_scan_limit(led_driver_max7219_handle_t handle, uint8_t chainId, uint8_t digits);



/**
 * @brief Set the operation mode on all MAX7219 / MAX7221 devices on the chain.
 *
 * @param[in] handle Handle to the MAX7219 / MAX7221 driver
 * @param[in] mode The mode to configure. See `max7219_mode_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_chain_mode(led_driver_max7219_handle_t handle, max7219_mode_t mode);

/**
 * @brief Set the operation mode on a specific MAX7219 / MAX7221 device on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAX7219 / MAX7221 driver
 * @param[in]  chainId Index of the MAX7219 / MAX7221 device to configure starting at 1 for the first device
 * @param[in]  mode The mode to configure. See `max7219_mode_t` for possible values 
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_mode(led_driver_max7219_handle_t handle, uint8_t chainId, max7219_mode_t mode);


/**
 * @brief Configure intensity on all MAX7219 / MAX7221 devices on the chain.
 * 
 * @param[in] handle Handle to the MAX7219 / MAX7221 driver
 * @param[in] intensity The duty cycle to set. See `max7219_intensity_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_chain_intensity(led_driver_max7219_handle_t handle, max7219_intensity_t intensity);

/**
 * @brief Set intensity on a specific MAX7219 / MAX7221 devices on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAX7219 / MAX7221 driver
 * @param[in]  chainId Index of the MAX7219 / MAX7221 device to configure starting at 1 for the first device
 * @param[in]  intensity The duty cycle to set. See `max7219_intensity_t` for possible values
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_intensity(led_driver_max7219_handle_t handle, uint8_t chainId, max7219_intensity_t intensity);



/**
 * @brief Set the given digit code on all digits of all MAX7219 / MAX7221 devices on the chain.
 * 
 * @param[in] handle Handle to the MAX7219 / MAX7221 driver
 * @param[in] digitCode The digit code to set. A `max7219_code_b_font_t` value for digits in Code B decode mode or a combination of `max7219_segment_t` values for devices in no decode mode
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_chain(led_driver_max7219_handle_t handle, uint8_t digitCode);

/**
 * @brief Set the given digit code on a MAX7219 / MAX7221 device on the chain.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAX7219 / MAX7221 driver
 * @param[in]  chainId Index of the MAX7219 / MAX7221 device to configure starting at 1 for the first device
 * @param[in]  digit The digit to set (1 to 8)
 * @param[in]  digitCode The digit code to set. A `max7219_code_b_font_t` value for digits in Code B decode mode or a combination of `max7219_segment_t` values for devices in no decode mode
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_digit(led_driver_max7219_handle_t handle, uint8_t chainId, uint8_t digit, uint8_t digitCode);

/**
 * @brief Set the given digit codes to MAX7219 / MAX7221 device on the chain, starting at the given device and digit.
 * 
 * @note The chain is organized as follows:
 *          |  Device 1  |  |  Device 2  |  |  Device 3  | ... |  Device N  |
 *            Chain Id 1      Chain Id 2      Chain Id 3         Chain Id N
 * 
 * @param[in]  handle Handle to the MAX7219 / MAX7221 driver
 * @param[in]  startChainId Index of the MAX7219 / MAX7221 device where codes should start being sent to
 * @param[in]  startDigitId The digit to start sending codes from (1 to 8)
 * @param[in]  digitCodes An array of digit codes to send. A `max7219_code_b_font_t` value for digits in Code B decode mode or a combination of `max7219_segment_t` values for devices in no decode mode
 * @param[in]  digitCodesCount Number of digit codes in array 'digitCodes'
 *
 * @return
 *      - ESP_OK: Success
 *      - ESP_ERR_INVALID_ARG: Invalid argument
 *      - ESP_ERR_INVALID_STATE: The driver is in an invalid state
 */
esp_err_t led_driver_max7219_set_digits(led_driver_max7219_handle_t handle, uint8_t startChainId, uint8_t startDigitId, const uint8_t digitCodes[], uint8_t digitCodesCount);



#ifdef __cplusplus
}
#endif