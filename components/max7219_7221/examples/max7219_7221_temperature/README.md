# Sample: Display ESP32 Temperature on Several Drivers

This sample demonstrates how to initialize the driver with three cascaded MAX7219 / MAX7221 devices and display the current temperature, minimum temperature and maximum temperature.

## Walk through
This sample demonstrates the following capabilities:
0. Initialize the ESP temperature sensor via `temperature_sensor_install()` and `temperature_sensor_enable()`,
1. Initialize an SPI host in master mode using ESP-IDF `spi_bus_initialize()`,
2. Initialize the MAX7219 / MAX7221 driver via `led_driver_max7219_init()`,
3. Configure scan limit to eight digits with `led_driver_max7219_configure_chain_scan_limit()`,
4. Set 'Direct Addressing' decoding using `led_driver_max7219_configure_chain_decode()`,
5. Set LEDs intensity to `MAX7219_INTENSITY_DUTY_CYCLE_STEP_2` using `led_driver_max7219_set_chain_intensity()`,
6. Switch between shutdown mode and normal mode with `led_driver_max7219_set_chain_mode()`,
7. Read the current temperature via `temperature_sensor_get_celsius()`, calculate the minimum and maximum,
8. Display the current, minimum and maximum temperature with `led_driver_max7219_set_digits()`,
9. Shutdown the MAX7219 / MAX7221 driver and free up resources it allocated via `led_driver_max7219_free()`.

## Hardware
To make the most out of the sample, the following hardware setup is recommended:

1. An ESP32 family device equipped with a temperature sensor. This sample has been tested with ESP32, ESP32-S3 and ESP32-C3. Other members of the ESP32 family should also work,
2. A power source capable of providing the MAX7219 / MAX7221 with a maximum of 6V at 500mA. See the MAX7219 / MAX7221 data sheet for electrical characteristics,
3. Three MAX7219 / MAX7221 devices each connected to eight seven-segment displays with decimal point. The rightmost digit is assumed to be wired as digit '0' on the MAX7219 / MAX7221.
4. Connect the first MAX7219 / MAX7221 `DOUT` to the second MAX7219 / MAX7221 `DIN` and `DOUT` from the second MAX7219 / MAX7221 to `DIN` of the third MAX7219 / MAX7221 device,
5. Connect all devices `LOAD/CS` and `CLK`. When cascading MAX7219 / MAX7221 devices, all devices share the same SPI `LOAD/CS` and `CLK`. Refer to the MAX7219 / MAX7221 data sheet for more information on cascading devices,
6. A 3-wire SPI connection (CLK, MOSI, /CS) between the MAX7219 / MAX7221 chain and the ESP32. **MAX7219 / 7221 devices cannot be reliably driven directly by 3.3V logic** due to their higher than normal minimum logic high voltage (Vih) requirement of 3.5V. There are several ways to overcome this challenge. Here are a few options, among many others:
    * Connect the ESP32 SPI lines (CLK, MOSI, /CS) to the first MAX7219 / MAX7221 in the chain via a logic level shifter (for instance a TXS0108E),
    * Connect the ESP32 SPI lines (CLK, MOSI, /CS) to the first MAX7219 / MAX7221 in the chain via a 3.3V compatible logic buffer (for instance a 74HCT125 / 74AHCT125),
    * Microchip [3V Tips ‘n Tricks](https://ww1.microchip.com/downloads/en/DeviceDoc/41285A.pdf) offers a few alternatives including connection via discrete N-Channel Logic Level Enhancement Mode FET circuitry (for instance BSS138), diode clamps, ...

## Firmware
In `max7219_7221_cascade.c`, configure `CS_LOAD_PIN` (`/CS`), `CLK_PIN` (`CLK`) and `DIN_PIN` (`MOSI`) adequately for your hardware setup:
```c
const gpio_num_t CS_LOAD_PIN = GPIO_NUM_19;
const gpio_num_t CLK_PIN = GPIO_NUM_18;
const gpio_num_t DIN_PIN = GPIO_NUM_16;
```

Build and flash an ESP32 device. Ensure you have a working connection to UART as the sample emits various information via ESP_LOGxxx.