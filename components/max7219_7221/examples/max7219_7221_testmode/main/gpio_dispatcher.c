// -----------------------------------------------------------------------------------
// Copyright 2024, Gilles Zunino
// -----------------------------------------------------------------------------------

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <esp_check.h>


#include "gpio_dispatcher.h"


static const char* GpioIsrTag = "gpio_isr";

static QueueHandle_t s_gpio_isr_dispatch_queue = NULL;


esp_err_t gpio_events_queue_dispatch(void) {
    isr_handler_fn_ptr handlerFn = NULL;
    BaseType_t dequeueOutcome = xQueueReceive(s_gpio_isr_dispatch_queue, &handlerFn, portMAX_DELAY);
    if (dequeueOutcome == pdTRUE) {
        if (handlerFn != NULL) {
            handlerFn();
        } else {
            ESP_LOGE(GpioIsrTag, "xQueueReceive() retrieved and ISR request with NULL handler");
        }
    } else {
        ESP_LOGE(GpioIsrTag, "xQueueReceive() failed (0x%x) - ISR will be dropped", dequeueOutcome);
    }
    return ESP_OK;
}

static void gpio_isr_handler(void* arg) {
    isr_handler_fn_ptr handlerFn = (isr_handler_fn_ptr) arg;
    BaseType_t enqueuOutcome = xQueueGenericSendFromISR(s_gpio_isr_dispatch_queue, &handlerFn, NULL, queueSEND_TO_BACK);
    if (enqueuOutcome != pdPASS) {
        ESP_DRAM_LOGE(GpioIsrTag, "xQueueGenericSendFromISR() failed (0x%x) - ISR will be dropped", enqueuOutcome);
    }
}

static esp_err_t configure_gpio_isr_queue() {
    s_gpio_isr_dispatch_queue = xQueueCreate(10, sizeof(void *));
    return s_gpio_isr_dispatch_queue != NULL ? ESP_OK : ESP_FAIL;
}

static esp_err_t shutdown_gpio_isr_queue() {
    if (s_gpio_isr_dispatch_queue != NULL) {
        vQueueDelete(s_gpio_isr_dispatch_queue);
        s_gpio_isr_dispatch_queue = NULL;
    }

    return ESP_OK;
}

esp_err_t ht_gpio_isr_handler_add(gpio_num_t gpioNum, isr_handler_fn_ptr fn) {
    if (fn != NULL) {
        return gpio_isr_handler_add(gpioNum, gpio_isr_handler, (void*) fn);
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t ht_gpio_isr_handler_delete(gpio_num_t gpioNum) {
    return gpio_isr_handler_remove(gpioNum);
}

esp_err_t configure_gpio_isr_dispatcher(void) {
    ESP_ERROR_CHECK(configure_gpio_isr_queue());

    //
    // Installing the GPIO ISR Service depends on IPC tasks - See https://docs.espressif.com/projects/esp-idf/en/v5.2.1/esp32/api-reference/system/ipc.html
    // The following configuration values are involved:
    // * CONFIG_ESP_IPC_USES_CALLERS_PRIORITY - Default on
    // * CONFIG_ESP_IPC_TASK_STACK_SIZE       - Default 1024 - Raised to 1280 (0x500) to avoid stack overflow in ipc0 (see sdkconfig.defaults)
    //
    const int ESP_INTR_FLAG_NONE = 0;
    esp_err_t err =  gpio_install_isr_service(ESP_INTR_FLAG_NONE);
    if (err != ESP_OK) {
        if (shutdown_gpio_isr_queue() != ESP_OK) {
            ESP_LOGE(GpioIsrTag, "gpio_install_isr_service() failed and cleanup action shutdown_gpio_isr_queue() failed (0x%x)", err);
        }
    }

    return err;
}

esp_err_t shutdown_gpio_isr_dispatcher() {
    esp_err_t err = shutdown_gpio_isr_queue();
    gpio_uninstall_isr_service();
    return err;
}