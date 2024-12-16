idf_component_register(
    SRCS "max7219_7221_temperature.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_tsens esp_driver_usb_serial_jtag max7219_7221
)