/*
 * Provides easy to use sleep() function.
 *
 * SPDX-License-Identifier: MIT
 */

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <rom/ets_sys.h>

#include <chrono>

#include "idfx/utils/log.hpp"

namespace idfx {

// So that can easily use std::chrono literals like 20ms
using namespace std::chrono_literals;

/**
 * Sleep for the specified number of microseconds. The sleep method is a bit
 * complicated. If the desired sleep time is less than FreeRTOS portTICK_PERIOD_MS
 * (10 msec)then use ets_delay_us() because it operates by busy-waiting, meaning the
 * CPU remains active and consumes power while delaying. This makes it suitable for
 * short, accurate delays where timing is critical. But if desired delay time is
 * greater than a FreeRTOS clock tick then this_thread::sleep_for() is used since
 * it yields to other tasks instead of hogging the CPU.
 *
 * @param microsecs The number of microseconds to sleep.
 */
void sleep(const std::chrono::duration<int, std::micro>& microsecs);

/**
 * Returns time since startup in microseconds.
 */
int64_t sinceStartupUsec();

}  // namespace idfx
