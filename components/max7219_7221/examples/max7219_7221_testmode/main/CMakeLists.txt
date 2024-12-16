idf_component_register(
    SRCS "gpio_dispatcher.c" "max7219_7221_testmode.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_usb_serial_jtag max7219_7221
)