/*
 * SPDX-License-Identifier: MIT
 */

#include "idfx/hardware/io.hpp"

#include <esp_private/esp_gpio_reserve.h>

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp-idf-cxx/gpio_cxx.hpp"
#include "idfx/utils/log.hpp"

// So that don't get warnings about the LEDC structures not being fully specified
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

using namespace idfx;

/********************************** OutputBit ***************************/

OutputBit::OutputBit(GPIONum num, std::string bit_name, IOExpander* io_expander_ptr)
    : pin_(num), bit_name_(bit_name), io_expander_ptr_(io_expander_ptr) {
    // Configure the pin as an output
    VERBOSE("Creating OutputBit for GPIO %d (%s)", pin_.get_value(), bit_name_.c_str());
    if (io_expander_ptr_) {
        io_expander_ptr_->configAsOutput(pin_.get_value());
    } else {
        DEBUG("Creating GPIO_Output for GPIO %d (%s)", pin_.get_value(), bit_name_.c_str());
        gpio_output_ptr_ = new GPIO_Output(pin_);
    }
}

// This constructor is just a convenience for when you don't want to specify a name
OutputBit::OutputBit(GPIONum num, IOExpander* io_expander_ptr_) : OutputBit(num, "", io_expander_ptr_) {
    // All work done in main constructor
}

bool OutputBit::get() const {
    if (gpio_output_ptr_) {
        // The GPIO_Output class doesn't have a way of reading the level directly,
        // so we need to use the underlying GPIO API to get the level.
        int level = gpio_get_level(pin_.get_value<gpio_num_t>());
        DEBUG("GPIO %d (%s) level is %d", pin_.get_value(), bit_name_.c_str(), level);
        return level == 1;
    } else if (io_expander_ptr_) {
        return io_expander_ptr_->getBit(pin_.get_value()) == 1;
    } else {
        ERROR("No valid GPIO or IOExpander to read from for pin %d (%s)", pin_.get_value(),
              bit_name_.c_str());
        return false;  // Default to false if we can't read the pin
    }
}

OutputBit::~OutputBit() {
    if (gpio_output_ptr_ != nullptr) {
        delete gpio_output_ptr_;
        gpio_output_ptr_ = nullptr;
    }
}

void OutputBit::setOn() const {
    INFO("Setting Output bit %d (%s) to HIGH", pin_.get_value(), bit_name_.c_str());

    // Set the GPIO or IOExpander bit to high
    if (gpio_output_ptr_) {
        DEBUG("Setting GPIO_Output for GPIO %d (%s) to HIGH", pin_.get_value(), bit_name_.c_str());
        gpio_output_ptr_->set_high();
    } else if (io_expander_ptr_) {
        io_expander_ptr_->setBit(pin_.get_value(), true);
    } else {
        // else: do nothing, as we don't have a valid GPIO or IOExpander
        ERROR("No valid GPIO or IOExpander to set for pin %d (%s)", pin_.get_value(),
              bit_name_.c_str());
    }
}

void OutputBit::setOff() const {
    if (gpio_output_ptr_) {
        DEBUG("Setting Output bit %d (%s) to LOW", pin_.get_value(), bit_name_.c_str());
        gpio_output_ptr_->set_low();
    } else if (io_expander_ptr_) {
        io_expander_ptr_->setBit(pin_.get_value(), false);
    } else {
        // else: do nothing, as we don't have a valid GPIO or IOExpander
        ERROR("No valid GPIO or IOExpander to set for pin %d (%s)", pin_.get_value(),
              bit_name_.c_str());
    }
}

/********************************** InputBit ***************************/

InputBit::InputBit(GPIONum num, std::string bit_name, IOExpander* io_expander_ptr)
    : pin_(num), bit_name_(bit_name), io_expander_ptr_(io_expander_ptr) {
    VERBOSE("Creating Input bit for GPIO %d (%s)", pin_.get_value(), bit_name_.c_str());

    // Configure the pin as an output
    if (io_expander_ptr_) {
        io_expander_ptr_->configAsInput(pin_.get_value());
    } else {
        DEBUG("Creating GPIOInput for GPIO %d (%s)", pin_.get_value(), bit_name_.c_str());
        gpioInput_ = new GPIOInput(pin_);
    }
}

// This constructor is just a convenience for when you don't want to specify a name
InputBit::InputBit(GPIONum num, IOExpander* io_expander_ptr_) : InputBit(num, "", io_expander_ptr_) {
    // All work done in main constructor
}

InputBit::~InputBit() {
    if (gpioInput_ != nullptr) {
        delete gpioInput_;
        gpioInput_ = nullptr;
    }
}

bool InputBit::get() const {
    if (gpioInput_) {
        DEBUG("Getting Input bit %d (%s)", pin_.get_value(), bit_name_.c_str());
        return gpioInput_->get_level() == GPIOLevel::HIGH;
    } else if (io_expander_ptr_) {
        return io_expander_ptr_->getBit(pin_.get_value()) == 1;
    } else {
        ERROR("No valid GPIO or IOExpander to read from for pin %d (%s)", pin_.get_value(),
              bit_name_.c_str());
        return false;  // Default to false if we can't read the pin
    }
}

/********************************** PWMTimer ***************************/

static auto timers_in_use = std::map<ledc_timer_t, PWMTimer*>();

/* static */
PWMTimer* PWMTimer::getAvailableTimer(const uint32_t freq_hz) {
    for (int num = LEDC_TIMER_0; num < LEDC_TIMER_MAX; ++num) {
        ledc_timer_t timer_num = static_cast<ledc_timer_t>(num);
        if (timers_in_use.find(timer_num) == timers_in_use.end()) {
            DEBUG("Found that timer num %ld is available", timer_num);
            // Found a not in use timer number. Therefore create it
            PWMTimer* new_timer_ptr = new PWMTimer(timer_num, freq_hz);

            // Remember that created it
            timers_in_use[timer_num] = new_timer_ptr;

            // Done
            return new_timer_ptr;
        }
    }

    // Couldn't find an unused timer
    return nullptr;
}

/* static */
PWMTimer* PWMTimer::getTimer(const ledc_timer_t timer_num, const uint32_t freq_hz) {
    DEBUG("Getting PWMTimer for timer number %ld", timer_num);

    auto found_timer = timers_in_use.find(timer_num);
    if (found_timer != timers_in_use.end()) {
        // timer already exists. Therefore increment reference count and return it.
        DEBUG("Returning existing PWMTimer for timer number %ld", timer_num);
        PWMTimer* timer_ptr = found_timer->second;
        timer_ptr->num_references_++;
        return timer_ptr;
    } else {
        // Timer doesn't already exist so create it
        DEBUG("Creating new PWMTimer for timer number %ld", timer_num);
        PWMTimer* new_timer_ptr = new PWMTimer(timer_num, freq_hz);
        return new_timer_ptr;
    }
}

void PWMTimer::doneWithTimer() {
    DEBUG("Done with reference to PWMTimer for timer number %ld", timer_num_);

    // Decrement reference count. If no more references to it then destruct the timer
    if (--num_references_ == 0) {
        // Update timers_in_use to indicate that this timer number is now available
        timers_in_use.erase(timer_num_);

        // Destruct the timer
        DEBUG("No more references to PWMTimer for timer number %ld so deleting it", timer_num_);
        delete this;
    } else {
        DEBUG("Still num_references_=%d for PWMTimer so not destroying it", num_references_);
    }
}

PWMTimer::PWMTimer(const ledc_timer_t timer_num, const uint32_t freq_hz)
    : timer_num_(std::clamp(timer_num, LEDC_TIMER_0, (ledc_timer_t)(LEDC_TIMER_MAX - 1))),
      // Oddly, for the ESP32S3 at least there is only low speed mode. High speed mode,
      // where harware is used and duty changes are glitch free, is simply not available.
      speed_mode_(LEDC_LOW_SPEED_MODE),
      freq_hz_(freq_hz) {
    DEBUG("Constructing PWMTimer for timer_num=%d and freq_hz=%ld", timer_num_, freq_hz_);

    // Initalize members. Remember that this one is in use by setting reference count to 1
    num_references_ = 1;

    if (timer_num_ != timer_num) {
        WARN(
            "PWMTimer must use a timer >= 0 and < LEDC_TIMER_MAX but was configured to be %d. "
            "Therefore set to %d",
            timer_num, timer_num_);
    }

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = speed_mode_,
        .duty_resolution = LEDC_TIMER_12_BIT,  // Duty can be 0 to 2^12=4096
        .timer_num = timer_num_,
        .freq_hz = freq_hz_,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

PWMTimer::~PWMTimer() {
    DEBUG("In destructor ~PWMTimer() for timer %ld", timer_num_);

    // First need to pause the timer
    ledc_timer_pause(speed_mode_, timer_num_);

    // Release the timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = speed_mode_, .timer_num = timer_num_, .deconfigure = true};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

void PWMTimer::setFrequency(const uint32_t freq_hz) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = speed_mode_,
        .duty_resolution = LEDC_TIMER_12_BIT,  // Duty can be 0 to 2^12=4096
        .timer_num = timer_num_,
        .freq_hz = freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

/********************************** OutputPWM ***************************/

// For keeping track of which channels are being used.
// Only used internally so declared as a static.
static auto channels_used = std::set<ledc_channel_t>();

// Calls the other constructor using get_available_channel() to properly determine
// channel number to use.
OutputPWM::OutputPWM(gpio_num_t gpio_num) : OutputPWM(gpio_num, get_available_channel()) {}

OutputPWM::OutputPWM(gpio_num_t gpio_num, ledc_channel_t channel)
    : timer_ptr_(PWMTimer::getAvailableTimer()),
      gpio_num_(gpio_num),
      channel_(channel),
      speed_mode_(timer_ptr_->getSpeedMode()) {
    INFO("Constructing OutputPWM for gpio_num=%d timer=%ld channel=%d", gpio_num_,
         timer_ptr_->getTimer(), channel_);

    // Keep track that this channel is being used
    channels_used.insert(channel);

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {.gpio_num = gpio_num_,
                                          .speed_mode = speed_mode_,
                                          .channel = channel_,
                                          .intr_type = LEDC_INTR_DISABLE,
                                          .timer_sel = timer_ptr_->getTimer(),
                                          .duty = 0,  // Set duty to 0% at initialization
                                          .hpoint = 0,
                                          .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
                                          .flags = false};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

OutputPWM::~OutputPWM() {
    INFO("Deleting OutputPWM for gpio_num=%d and channel=%d", gpio_num_, channel_);

    // Release the reference to the timer
    timer_ptr_->doneWithTimer();

    // Oddly, there is an ESP-IDF bug where GPIO LEDC bits are reserved automatically,
    // but never released. Therefore releasing it manually. Unfortunately have to
    // include the private <esp_private/esp_gpio_reserve.h> header file to do this.
    esp_gpio_revoke(BIT64(gpio_num_));

    // Keep track that this channel no longer being used
    channels_used.erase(channel_);
}

ledc_channel_t OutputPWM::get_available_channel(void) {
    DEBUG("Determining available LEDC channel...");

    for (int num = LEDC_CHANNEL_0; num < LEDC_CHANNEL_MAX; ++num) {
        ledc_channel_t channel_num = static_cast<ledc_channel_t>(num);
        if (!channels_used.contains(channel_num)) {
            DEBUG("Will be using LEDC channel %ld", channel_num);
            // Found channel not already used
            return channel_num;
        }
    }

    // There was no available channel!
    return LEDC_CHANNEL_MAX;
}

void OutputPWM::setDuty(const float percentage) {
    DEBUG("Setting duty for GPIO PWM bit %ld to %f%%", gpio_num_, percentage);

    setDutyValue(percentage * PWMTimer::MAX_DUTY / 100.0);
}

void OutputPWM::setDutyValue(const uint32_t duty) {
    if (duty > PWMTimer::MAX_DUTY) {
        WARN(
            "For GPIO PWM bit %d tried to set duty to %ld but maximum duty is %ld so has been set "
            "to that value",
            gpio_num_, duty, PWMTimer::MAX_DUTY);
        duty_ = PWMTimer::MAX_DUTY;
    } else {
        duty_ = duty;
    }
    DEBUG("Setting OutputPWM bit %d on channel %ld to %ld out of %ld", gpio_num_, channel_, duty_,
          PWMTimer::MAX_DUTY);

    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(speed_mode_, channel_, duty_));

    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(speed_mode_, channel_));
}

void OutputPWM::setFrequency(const uint32_t freq_hz) {
    timer_ptr_->setFrequency(freq_hz);

    // Since changing frequency of a timer also changes the duty of the signal
    // the duty is reset to its original value.
    setDutyValue(duty_);
}
