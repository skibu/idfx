/**
 * For handling GPIO based interrupts. Uses freeRTOS tasks so that the heavy lifting
 * is done at the appropriate time, when the specially created GPIO interrupts task
 * is enabled and gets a message from the queue.
 *
 * Note: to use this feature needed to increase the config parameter
 * "Inter-Processor Call (IPC) task stack size" from default of 1280 to 2560 in order
 * to prevent problem with the "ipc0" stack.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp-idf-cxx/gpio_cxx.hpp"

namespace idfx {

// Type definition for the isr function to be called by GPIO task when GPIO bit 
// detects an interrupt.
typedef void (*isr_function_t)(int);

/**
 * For configuring an input GPIO bit as an interrupt so that a specified
 * static function is called.
 *
 * GpioInterrupteHandler is a singleton. Therefore all members are static.
 */
class GpioInterrupteHandler {
   public:
    /**
     * Configures the specified GPIO interrupt bit to call individual_isr_for_bit()
     * when triggered.
     * @param gpio_num The GPIO pin number.
     * @param individual_isr_for_bit function pointer to function to be called when
     * bit receives interrupt signal.
     * @param intr_type Type of interrupt. A gpio_int_type_t so can be GPIO_INTR_DISABLE,
     * GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE, GPIO_INTR_ANYEDGE, GPIO_INTR_LOW_LEVEL, or
     * GPIO_INTR_HIGH_LEVEL. Default is GPIO_INTR_POSEDGE.
     * @param pull_up_en Whether pull up resister should be enabled. A gpio_pullup_t so
     * can be GPIO_PULLUP_DISABLE or GPIO_PULLUP_ENABLE. Default is GPIO_PULLUP_DISABLE
     * @param pull_down_en Whether pull down resister should be enabled. A gpio_pulldown_t so
     * can be GPIO_PULLDOWN_DISABLE or GPIO_PULLDOWN_ENABLE. Default is GPIO_PULLDOWN_ENABLE
     */
    GpioInterrupteHandler(GPIONum gpio_num,
                          isr_function_t individual_isr_for_bit,
                          gpio_int_type_t intr_type = GPIO_INTR_POSEDGE,
                          gpio_pullup_t pull_up_en = GPIO_PULLUP_DISABLE,
                          gpio_pulldown_t pull_down_en = GPIO_PULLDOWN_ENABLE);
};

}  // end of namespace idfx