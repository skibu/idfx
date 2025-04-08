/*
 * SPDX-License-Identifier: MIT
 */

#include "idfx/utils/log.hpp"

#include "freertos/FreeRTOS.h" // FIXME needed?
#include "freertos/task.h" // FIXME needed?


std::string idfx::fullThreadId() {
    // Note: For ESP cannot use regular pthread info for the "main" thread.
    // This is due to FreeRTOS not using pthreads. Therefore using FreeRTOS
    // functions to get the current task's name.
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    char *thread_name =  pcTaskGetName(task_handle);
    return std::string(thread_name);
}

std::string idfx::trimAndPadLeft(const std::string& input, int length, char paddingChar) {
    std::stringstream ss;
    ss << std::setw(length) << std::setfill(paddingChar) << std::right << input.substr(0, length);
    return ss.str();
}

std::string idfx::trimAndPadRight(const std::string& input, int length, char paddingChar) {
    std::stringstream ss;
    ss << std::setw(length) << std::setfill(paddingChar) << std::left << input.substr(0, length);
    return ss.str();
}
