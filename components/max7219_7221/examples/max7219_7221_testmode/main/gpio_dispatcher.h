// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#pragma once

#include <esp_err.h>
#include <driver/gpio.h>


typedef void (*isr_handler_fn_ptr)();

esp_err_t ht_gpio_isr_handler_add(gpio_num_t gpioNum, isr_handler_fn_ptr fn);
esp_err_t ht_gpio_isr_handler_delete(gpio_num_t gpioNum);

esp_err_t configure_gpio_isr_dispatcher();
esp_err_t shutdown_gpio_isr_dispatcher();

esp_err_t gpio_events_queue_dispatch();