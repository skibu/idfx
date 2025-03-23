/*
 * SPDX-License-Identifier: MIT
 */

#include "log.hpp"

#include "freertos/FreeRTOS.h" // FIXME needed?
#include "freertos/task.h" // FIXME needed?


std::string idfx::threadId() {
    // Note: For ESP cannot use regular pthread info for the "main" thread.
    // This is due to FreeRTOS not using pthreads. Therefore using FreeRTOS
    // functions to get the current task's name.
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    char *thread_name =  pcTaskGetName(task_handle);
    return std::string(thread_name);
}

std::string idfx::shortThreadId() {
    const int MAX_CHARS = 8;
    std::string id = threadId();
    if (id.size() < MAX_CHARS) {
        return id;
    } else {
        return id.substr(id.size() - MAX_CHARS);
    }
}

