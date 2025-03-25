/*
 * C++ classes for handling interrupt GPIO
 *
 * SPDX-License-Identifier: MIT
 */

#include "idfx/hardware/interrupts.hpp"

#include <rom/ets_sys.h>

#include <map>

#include "esp_intr_alloc.h"
#include "idfx/utils/log.hpp"

// Structure for data in the queue. Identifies the GPIO bit and the ISR function to call.
typedef struct QueueData {
    int gpio_num;
    idf::isr_function_t individual_isr_for_bit;
} queue_object_t;

// Forward declaration
static void gpioIsrTaskFunction(void *arg);

/**
 * Initializes the GPIO interrupt handling if haven't done so yet.
 */
static void initializeIfNeeded(void) {
    // If already done this, don't need to do again
    static bool initialized = false;
    if (initialized) return;

    // start gpio task
    UBaseType_t UX_PRIORITY = 10;
    void *NULL_PV_PARAMETERS = nullptr;
    TaskHandle_t *NULL_CREATED_TASK = nullptr;
    xTaskCreate(gpioIsrTaskFunction,
                "gpio_isr_task",
                4096,  // FIXME Note: the usual 2048 resulted in stack overflow
                NULL_PV_PARAMETERS, UX_PRIORITY, NULL_CREATED_TASK);

    // So that only initialize once
    initialized = true;
}

/* The internal interrupt handler communicates with the xTask using this queue.
 * the IO bit number is passed via the queue so that the proper interrupt handler
 * for the bit can be identified and called. */
static QueueHandle_t gpio_event_queue_ = xQueueCreate(10, sizeof(queue_object_t));

/* Map for containing for each GPIO bit the ISR function to be called by the GPIO
 * task when it is appropriate time to handle the interrupt. Keyed on integer GPIO
 * bit number and values are QueueData objects containing bit number and isr function ptr.
 */
static std::map<int, QueueData> gpio_bit_data_map_ = std::map<int, QueueData>();

/**
 * The single freeRTOS Task that runs continuously to do actual processing of GPIO interrupts.
 * Reads from the GPIO interrupts queue and calls the isr configured for the IO bit.
 * @param *arg Pointer to arg specified when xTaskCreate() called in order to setup
 * the GPIO isr task. Not actually used.
 */
static void gpioIsrTaskFunction(void *arg) {
    INFO("Running task gpio_isr_task forever...");

    // Install gpio isr service. This must be done in the task that has been
    // created to handle the interrupts. Note that have to use intr_alloc_flags
    // of ESP_INTR_FLAG_LOWMED because if use a higher level then the ISR must
    // be written in assembly language. Otherwise get a very cryptic alloc error
    // message.
    esp_err_t result;
    if (ESP_OK == (result = gpio_install_isr_service(ESP_INTR_FLAG_LOWMED))) {
        DEBUG("Successfully initialized per bit interrupts via gpio_install_isr_service()");
    } else {
        ERROR("Error occurred in gpio_install_isr_service(). Returned 0x%X", result);
    }

    QueueData data;
    for (;;) {
        // Wait until interrupt event is received
        if (xQueueReceive(gpio_event_queue_, &data, portMAX_DELAY)) {
            // Got from the queue a GPIO interrupt to handle
            int io_num = data.gpio_num;
            idf::isr_function_t isr_func = data.individual_isr_for_bit;
            DEBUG("gpio_isr_task_function() Task handling interrupt. GPIO[%" PRIu32 "] intr, val: %d",
                       io_num, gpio_get_level(static_cast<gpio_num_t>(io_num)));

            // Call the user defined isr that was defined for this GPIO interrupt.
            // And pass in the IO bit number.
            DEBUG("About to call the user ISR...");
            (*isr_func)(io_num);
            DEBUG("Called the user ISR!");
        } else {
            INFO("xQueueReceive() did not return any data so will try again...");
        }
    }
}

/**
 * The internal interrupt handler. It is intended to be as short as possible since other
 * tasks maybe supposed to get priority. It simply puts an event into the xQueue so that
 * the xTask can handle the interrupt at appropriate time, and then call the
 * appropriate static function that was configured for the interrupt.
 * @param *arg the gpio number of the interrupt bit.
 */
static void IRAM_ATTR gpioIsrHandler(void *arg) {
    // We can get bit number since we setup ISR using gpio_install_isr_service(ESP_INTR_FLAG_LEVELMASK)
    uint32_t gpio_num = (uint32_t)arg;

    // Log using TASK_DEBUG() since regular logging statements should not be called in an ISR
    TASK_DEBUG("Internal ISR for bit %ld called. Adding even to queue.", GPIO_NUM_0);

    // Create the data that describes the interrupt handler
    QueueData queue_data = gpio_bit_data_map_[gpio_num];

    // Add to the Queue the queue data that describies how to handle the interrupt
    xQueueSendFromISR(gpio_event_queue_, &queue_data, NULL);
}

idfx::GpioInterrupteHandler::GpioInterrupteHandler(idf::GPIONum gpio_num,
                           isr_function_t individual_isr_for_bit,
                           gpio_int_type_t intr_type,
                           gpio_pullup_t pull_up_en,
                           gpio_pulldown_t pull_down_en) {
    // Makes sure one-time initialization has been done
    initializeIfNeeded();

    // Convert GPIONum into more useful type gpio_num_t
    auto bit_num = static_cast<gpio_num_t>(gpio_num.get_value());

    DEBUG("Initializing interrupt handling for GPIO bit %d intr_type=%d pull_up_en=%d pull_down_en=%d",
          bit_num, intr_type, pull_up_en, pull_down_en);

    // Setup config structure for the GPIO bit.
    // Must be an input, but configured as in/out via GPIO_MODE_INPUT_OUTPUT so that can test the
    // interrupt by setting the level of the output bit.
    const uint64_t bit_mask = 1ULL << bit_num;
    gpio_config_t io_conf = {
        .pin_bit_mask = bit_mask,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = pull_up_en,
        .pull_down_en = pull_down_en,
        .intr_type = intr_type};

    // Config the GPIO bit
    gpio_config(&io_conf);

    // Add data for this bit to the map so that it can be accessed later, when the isr is actually triggered.
    gpio_bit_data_map_[bit_num] = QueueData{.gpio_num = bit_num, .individual_isr_for_bit = individual_isr_for_bit};

    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(static_cast<gpio_num_t>(bit_num), gpioIsrHandler, (void *)bit_num);
}
