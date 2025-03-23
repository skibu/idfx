/* 
 * Utilities to improve logging. Uses esp_log.h functionality so that logging level
 * can be set as usual. Key improvements is that don't need to specify TAG and
 * for LOGD() and LOGE() additional context is provided. The additional context
 * is the function name, file name, line number, and thread ID.
 *
 * SPDX-License-Identifier: MIT
 */

#include <esp_debug_helpers.h>  // So cqn output stacktrace for error logging
#include <esp_log.h>

#include <cstdio>
#include <filesystem>

namespace stdx {

/* Returns the current thread's ID as a string */
std::string threadId();

/* Returns the current thread's ID as a string, but only the last 8 characters */
std::string shortThreadId();

}  // namespace stdx

/* Provides the thread name as the TAG used for logging */
#define TAG_FOR_LOGGING (std::string("t=") + utils::short_thread_id()).c_str()

/* Verbose macro. Uses ESP_LOGV() but adds additional info */
#define VERBOSE(format, ...) ESP_LOGV(TAG_FOR_LOGGING, "%s() %s:%d " format,              \
                                      __func__,                                           \
                                      std::filesystem::path(__FILE__).filename().c_str(), \
                                      __LINE__, ##__VA_ARGS__)

/* Debugging macro. Uses ESP_LOGD() but adds additional info like thread id, filename,
line number, and function name */
#define DEBUG(format, ...) ESP_LOGD(TAG_FOR_LOGGING, "%s() %s:%d " format,              \
                                    __func__,                                           \
                                    std::filesystem::path(__FILE__).filename().c_str(), \
                                    __LINE__, ##__VA_ARGS__)

/* For executing code, like timing code, only if in debug mode */
#define DEBUGGING(code)                                           \
    if (LOG_LOCAL_LEVEL >= ESP_LOG_GET_LEVEL(ESP_LOG_DEBUG)) do { \
            code;                                                 \
    } while (0)

/* Info macro. Uses ESP_LOGI() but adds additional info like thread id, filename,
line number, and function name. At some point might not want this extra info for
INFO logging and would then just call ESP_LOGI(). */
#define INFO(format, ...) ESP_LOGI(TAG_FOR_LOGGING, "%s() %s:%d " format,              \
    __func__,                                           \
    std::filesystem::path(__FILE__).filename().c_str(), \
    __LINE__, ##__VA_ARGS__)

// Here is how could simplify INFO() to just call ESP_LOGI()    
//#define INFO(format, ...) ESP_LOGI(FILE_NAME_AS_TAG, format, ##__VA_ARGS__)

/* Warning macro. Uses ESP_LOGW() but adds additional info like thread id, filename,
line number, and function name */
#define WARN(format, ...) ESP_LOGW(TAG_FOR_LOGGING, "%s() %s:%d " format,              \
                                    __func__,                                           \
                                    std::filesystem::path(__FILE__).filename().c_str(), \
                                    __LINE__, ##__VA_ARGS__)

/* Error macro. Uses ESP_LOGE() but adds additional info. Also prints the stack trace.
Unfortunaly this will output the stack trace even if error logging not enabled. But that
should be okay since likely always will have error logging enabled. */
#define ERROR(format, ...)                                           \
    do {                                                             \
        ESP_LOGE(TAG_FOR_LOGGING, "%s() %s:%d " format,              \
                 __func__,                                           \
                 std::filesystem::path(__FILE__).filename().c_str(), \
                 __LINE__, ##__VA_ARGS__);                           \
        esp_backtrace_print(12);                                     \
    } while (0)

/* For if wanted to use the filename of the current file as TAG. This function needs to be
   in the hpp file so that it uses the correct file name via the __FILE__ macro. */
#define FILE_NAME_AS_TAG std::filesystem::path(__FILE__).filename().c_str()

/* Regular logging cannot be done within freeRTOS tasks. The system panics and reboots,
 * which is quite ugly. Therefore need special logging statements for within tasks. But
 * can use ets_printf()! Still need to keep these logging statements simple since they
 * will be running within a task that has very limited stack space. Therefore not outputting
 * time, function name, file name, and line number.
 */
#define TASK_DEBUG(format, ...)                                   \
    if (LOG_LOCAL_LEVEL >= ESP_LOG_GET_LEVEL(ESP_LOG_DEBUG)) do { \
            ets_printf("D - " format "\n", ##__VA_ARGS__);        \
    } while (0)

#define TASK_INFO(format, ...)                                   \
    if (LOG_LOCAL_LEVEL >= ESP_LOG_GET_LEVEL(ESP_LOG_INFO)) do { \
            ets_printf("I - " format "\n", ##__VA_ARGS__);       \
    } while (0)

#define TASK_WARN(format, ...)                                   \
    if (LOG_LOCAL_LEVEL >= ESP_LOG_GET_LEVEL(ESP_LOG_WARN)) do { \
            ets_printf("W - " format "\n", ##__VA_ARGS__);       \
    } while (0)

#define TASK_ERROR(format, ...)                                   \
    if (LOG_LOCAL_LEVEL >= ESP_LOG_GET_LEVEL(ESP_LOG_ERROR)) do { \
            ets_printf("E - " format "\n", ##__VA_ARGS__);        \
    } while (0)
