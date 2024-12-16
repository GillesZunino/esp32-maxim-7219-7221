A driver to control one or more [MAX7219 / MAX7221](https://www.analog.com/en/products/MAX7219.html) serially interfaced, 8-Digit, LED Display Drivers [![Component Registry](https://components.espressif.com/components/gilleszunino/max7219_7221/badge.svg)](https://components.espressif.com/components/gilleszunino/max7219_7221)

## Documentation
Read the [documentation](./components/max7219_7221/README.md).

## Samples
Browse samples:
Several samples are located under `examples`. This section lists samples and capabilities demonstrated. Refer to a sample `README.md` file for more details.
* [`max_7219_7221_basic`](./components/max7219_7221/examples/max7219_7221_basic/README.md) demonstrates how to initialize, configure a single MAX7219 / MAX7221 device and display digits. This is the sample to start from,
* [`max_7219_7221_cascade`](./components/max7219_7221/examples/max7219_7221_cascade/README.md) demonstrates how to initialize, configure a chain of three MAX7219 / MAX7221 cascaded devices and display digits,
* [`max_7219_7221_decode`](./components/max7219_7221/examples/max7219_7221_decode/README.md) demonstrates how to initialize, configure a single MAX7219 / MAX7221 device and set per digit decode mode,
* [`max_7219_7221_intensity`](./components/max7219_7221/examples/max7219_7221_intensity/README.md) demonstrates how to control display intensity,
* [`max_7219_7221_scanlimit`](./components/max7219_7221/examples/max7219_7221_scanlimit/README.md) demonstrates how to control scan limit (how many digits are active on a given MAX7219 / MAX7221 device),
* [`max_7219_7221_temperature`](./components/max7219_7221/examples/max7219_7221_temperature/README.md) demonstrates how to display the current ESP32 device temperature, minimum and maximum,
* [`max_7219_7221_testmode`](./components/max7219_7221/examples/max7219_7221_testmode/README.md) demonstrates how to control test mode.