/*
 * Note: this file is a copy of https://github.com/espressif/esp-idf-cxx/blob/main/gpio_cxx.cpp 
 * It is here since cannot include the component esp-idf-cxx since it uses the old v1 I2C driver.
 * Therefore this use useful class is copied manually here.
 * 
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if __cpp_exceptions

#include "esp-idf-cxx/gpio_cxx.hpp"

#include <array>

#include "driver/gpio.h"

namespace idfx {

#define GPIO_CHECK_THROW(err) CHECK_THROW_SPECIFIC((err), GPIOException)

namespace {
#if CONFIG_IDF_TARGET_LINUX
constexpr std::array<uint32_t, 1> INVALID_GPIOS = {24};
#elif CONFIG_IDF_TARGET_ESP32
constexpr std::array<uint32_t, 1> INVALID_GPIOS = {24};
#elif CONFIG_IDF_TARGET_ESP32S2
constexpr std::array<uint32_t, 4> INVALID_GPIOS = {22, 23, 24, 25};
#elif CONFIG_IDF_TARGET_ESP32S3
constexpr std::array<uint32_t, 4> INVALID_GPIOS = {22, 23, 24, 25};
#elif CONFIG_IDF_TARGET_ESP32C3
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#elif CONFIG_IDF_TARGET_ESP32C2
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#elif CONFIG_IDF_TARGET_ESP32C6
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#elif CONFIG_IDF_TARGET_ESP32H2
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#else
#error "No GPIOs defined for the current target"
#endif

}  // namespace

GPIOException::GPIOException(esp_err_t error) : ESPException(error) {}

esp_err_t check_gpio_pin_num(uint32_t pin_num) noexcept {
    if (pin_num >= GPIO_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    for (auto num : INVALID_GPIOS) {
        if (pin_num == num) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    return ESP_OK;
}

esp_err_t check_gpio_drive_strength(uint32_t strength) noexcept {
    if (strength >= GPIO_DRIVE_CAP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

GPIOPullMode GPIOPullMode::FLOATING() {
    return GPIOPullMode(GPIO_FLOATING);
}

GPIOPullMode GPIOPullMode::PULLUP() {
    return GPIOPullMode(GPIO_PULLUP_ONLY);
}

GPIOPullMode GPIOPullMode::PULLDOWN() {
    return GPIOPullMode(GPIO_PULLDOWN_ONLY);
}

GPIOWakeupIntrType GPIOWakeupIntrType::LOW_LEVEL() {
    return GPIOWakeupIntrType(GPIO_INTR_LOW_LEVEL);
}

GPIOWakeupIntrType GPIOWakeupIntrType::HIGH_LEVEL() {
    return GPIOWakeupIntrType(GPIO_INTR_HIGH_LEVEL);
}

GPIODriveStrength GPIODriveStrength::DEFAULT() {
    return MEDIUM();
}

GPIODriveStrength GPIODriveStrength::WEAK() {
    return GPIODriveStrength(GPIO_DRIVE_CAP_0);
}

GPIODriveStrength GPIODriveStrength::LESS_WEAK() {
    return GPIODriveStrength(GPIO_DRIVE_CAP_1);
}

GPIODriveStrength GPIODriveStrength::MEDIUM() {
    return GPIODriveStrength(GPIO_DRIVE_CAP_2);
}

GPIODriveStrength GPIODriveStrength::STRONGEST() {
    return GPIODriveStrength(GPIO_DRIVE_CAP_3);
}

GPIOBase::GPIOBase(GPIONum num) : gpio_num(num) {
    GPIO_CHECK_THROW(gpio_reset_pin(gpio_num.get_value<gpio_num_t>()));
}

void GPIOBase::hold_en() {
    GPIO_CHECK_THROW(gpio_hold_en(gpio_num.get_value<gpio_num_t>()));
}

void GPIOBase::hold_dis() {
    GPIO_CHECK_THROW(gpio_hold_dis(gpio_num.get_value<gpio_num_t>()));
}

void GPIOBase::set_drive_strength(GPIODriveStrength strength) {
    GPIO_CHECK_THROW(gpio_set_drive_capability(gpio_num.get_value<gpio_num_t>(),
                                               strength.get_value<gpio_drive_cap_t>()));
}

GPIO_Output::GPIO_Output(GPIONum num) : GPIOBase(num) {
    GPIO_CHECK_THROW(gpio_set_direction(gpio_num.get_value<gpio_num_t>(), GPIO_MODE_OUTPUT));
}

void GPIO_Output::set_high() const {
    GPIO_CHECK_THROW(gpio_set_level(gpio_num.get_value<gpio_num_t>(), 1));
}

void GPIO_Output::set_low() const {
    GPIO_CHECK_THROW(gpio_set_level(gpio_num.get_value<gpio_num_t>(), 0));
}

GPIODriveStrength GPIOBase::get_drive_strength() {
    gpio_drive_cap_t strength;
    GPIO_CHECK_THROW(gpio_get_drive_capability(gpio_num.get_value<gpio_num_t>(), &strength));
    return GPIODriveStrength(static_cast<uint32_t>(strength));
}

GPIOInput::GPIOInput(GPIONum num) : GPIOBase(num) {
    GPIO_CHECK_THROW(gpio_set_direction(gpio_num.get_value<gpio_num_t>(), GPIO_MODE_INPUT));
}

GPIOLevel GPIOInput::get_level() const noexcept {
    int level = gpio_get_level(gpio_num.get_value<gpio_num_t>());
    if (level) {
        return GPIOLevel::HIGH;
    } else {
        return GPIOLevel::LOW;
    }
}

void GPIOInput::set_pull_mode(GPIOPullMode mode) const {
    GPIO_CHECK_THROW(
        gpio_set_pull_mode(gpio_num.get_value<gpio_num_t>(), mode.get_value<gpio_pull_mode_t>()));
}

void GPIOInput::wakeup_enable(GPIOWakeupIntrType interrupt_type) const {
    GPIO_CHECK_THROW(gpio_wakeup_enable(gpio_num.get_value<gpio_num_t>(),
                                        interrupt_type.get_value<gpio_int_type_t>()));
}

void GPIOInput::wakeup_disable() const {
    GPIO_CHECK_THROW(gpio_wakeup_disable(gpio_num.get_value<gpio_num_t>()));
}

GPIO_OpenDrain::GPIO_OpenDrain(GPIONum num) : GPIOInput(num) {
    GPIO_CHECK_THROW(
        gpio_set_direction(gpio_num.get_value<gpio_num_t>(), GPIO_MODE_INPUT_OUTPUT_OD));
}

void GPIO_OpenDrain::set_floating() const {
    GPIO_CHECK_THROW(gpio_set_level(gpio_num.get_value<gpio_num_t>(), 1));
}

void GPIO_OpenDrain::set_low() const {
    GPIO_CHECK_THROW(gpio_set_level(gpio_num.get_value<gpio_num_t>(), 0));
}

}  // namespace idfx

#endif