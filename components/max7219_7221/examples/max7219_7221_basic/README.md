# Sample: Basic Usage
This sample demonstrates how to initialize the driver with one MAX7219 / MAX7221 device, configure one MAX7219 / MAX7221 device and display Code-B or Direct Addressing symbols.

## Walk through
This sample demonstrates the following capabilities:
1. Initialize an SPI host in master mode using ESP-IDF `spi_bus_initialize()`,
2. Initialize the MAX7219 / MAX7221 driver via `led_driver_max7219_init()`,
3. Configure scan limit to eight digits with `led_driver_max7219_configure_chain_scan_limit()`,
4. Switch between shutdown mode and normal mode with `led_driver_max7219_set_chain_mode()`,
5. Switch between "Code-B" and "Direct Addressing" decoding using `led_driver_max7219_configure_chain_decode()`,
6. Display symbols using `led_driver_max7219_set_digit()`.
7. Shutdown the MAX7219 / MAX7221 driver and free up resources it allocated via `led_driver_max7219_free()`.

## Hardware
To make the most out of the sample, the following hardware setup is recommended:

1. An ESP32 family device. This sample has been tested with ESP32, ESP32-S3 and ESP32-C3. Other members of the ESP32 family should also work,
2. A power source capable of providing the MAX7219 / MAX7221 with a maximum of 6V at 500mA. See the MAX7219 / MAX7221 data sheet for electrical charactersistics,
3. A MAX7219 / MAX7221 device connected to eight seven segment displays with decimal point. The right most digit is assumed to be wired as digit '0' on the MAX7219 / MAX7221,
4. An 3-wire SPI connection (CLK, MOSI, /CS) between the MAX7219 / MAX7221 and the ESP32. **MAX7219 / 7221 devices cannot be reliably driven directly by 3.3V logic** due to their higher than normal minimum logic high voltage (Vih) requirement of 3.5V. There are several ways to overcome this challenge. Here are a few options, among many other:
    * Connect the ESP32 SPI lines (CLK, MOSI, /CS) to the MAX7219 / MAX7221 via a logic level shifter (for instance a TXS0108E),
    * Connect the ESP32 SPI lines (CLK, MOSI, /CS) to the MAX7219 / MAX7221 via a 3.3V compatible logic buffer (for instance a 74HCT125 / 74AHCT125),
    * Microchip [3V Tips ‘n Tricks](https://ww1.microchip.com/downloads/en/DeviceDoc/41285A.pdf) offers a few alternatives including connection via discrete N-Channel Logic Level Enhancement Mode FET circuitery (for instance BSS138), diode clamps, ...

## Firmware
In `max7219_7221_basic.c`, configure `CS_LOAD_PIN` (`/CS`), `CLK_PIN` (`CLK`) and `DIN_PIN` (`MOSI`) adequately for your hardware setup:
```c
const gpio_num_t CS_LOAD_PIN = GPIO_NUM_19;
const gpio_num_t CLK_PIN = GPIO_NUM_18;
const gpio_num_t DIN_PIN = GPIO_NUM_16;
```

Build and flash an ESP32 device. Ensure you have a working connection to UART as the sample emits various information via ESP_LOGxxx.