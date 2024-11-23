idf_component_register(SRCS "max7219_7221_temperature.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_tsens max7219_7221)