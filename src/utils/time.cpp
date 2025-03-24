/*
 * SPDX-License-Identifier: MIT
 */

#include "idfx/utils/time.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <rom/ets_sys.h>

#include <chrono>
#include <thread>

#include "idfx/utils/log.hpp"


using namespace std::chrono_literals;

void idfx::sleep(const std::chrono::duration<int, std::micro>& microsecs) {
    DEBUG("About to sleep for %lld microseconds...", (int64_t)microsecs.count());
    int64_t initial_time_microsecs;

    if (microsecs.count() > (portTICK_PERIOD_MS * 1000)) {
        // Longer delay so use this_thread::sleep_for() which yields control to other threads
        VERBOSE(
            "Sleeping for more than 1 tick period (portTICK_PERIOD_MS=%lu msec or %lu usec) "
            "so using std::this_thread::sleep_for() to yield control to other tasks",
            portTICK_PERIOD_MS, portTICK_PERIOD_MS * 1000);
        DEBUGGING(initial_time_microsecs = esp_timer_get_time());
        std::this_thread::sleep_for(microsecs);
    } else {
        // Shorter delay so use ets_delay_us() which hogs CPU but is more accurate
        VERBOSE("Sleeping for <= 1 tick period so using ets_delay_us() for best accuracy");
        DEBUGGING(initial_time_microsecs = esp_timer_get_time());
        ets_delay_us(microsecs.count());
    }
    VERBOSE("Slept for %lld microseconds", esp_timer_get_time() - initial_time_microsecs);
}

int64_t idfx::sinceStartupUsec() {
    return esp_timer_get_time();
}
