/**
 * Classes for handling GPIO pins and peripherals in a microcontroller.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include "driver/ledc.h"
#include "esp_err.h"
#include "gpio_cxx.hpp"
#include "idfx/hardware/ioExtender.hpp"
#include "idfx/utils/log.hpp"

namespace idfx {

/**
 * OutputBit class represents a GPIO pin configured as an output. Works for both pins on the
 * ESP32 and pins on an IO extender (like PCA9557). Provides methods to set and get the pin state.
 */
class OutputBit {
   public:
    /**
     * Constructor for when you want to specify a name for the output bit. This name
     * is used for logging and debugging purposes. The bit is configured as an output bit in
     * the constructor.
     * @param num The GPIO pin number.
     * @param name The name of the output bit (for logging).
     * @param ioExtender_ Pointer to an IOExtender object, if using one.
     */
    OutputBit(GPIONum num, std::string bit_name, IOExtender* ioExtender_ = nullptr);

    /**
     * Constructor for when don't want to specify a name for the output bit.
     * @param num The GPIO pin number.
     * @param ioExtender_ Pointer to an IOExtender object, if using one.
     */
    OutputBit(GPIONum num, IOExtender* ioExtender_ = nullptr);

    ~OutputBit();

    /**
     * Sets the output bit to GPIOLevel::HIGH
     */
    void setOn() const;

    /**
     * Sets the output bit to GPIOLevel::LOW
     */
    void setOff() const;

    /**
     * Sets the output bit to GPIOLevel::HIGH or GPIOLevel::LOW
     * @param on true to set HIGH, false to set LOW.
     */
    void set(bool on) const {
        if (on) {
            setOn();
        } else {
            setOff();
        }
    }

    /**
     * Sets the output bit to GPIOLevel::HIGH or GPIOLevel::LOW
     * @param level The desired GPIO level (HIGH or LOW).
     */
    void set(GPIOLevel level) const {
        if (level == GPIOLevel::HIGH) {
            setOn();
        } else {
            setOff();
        }
    }

    /**
     * Returns the current state of the output bit.
     * @return true if the pin is HIGH, false if it is LOW.
     */
    bool get() const;

   private:
    GPIONum pin_;
    std::string bit_name_;
    GPIO_Output* gpioOutput_ = nullptr;  // Managed by the destructor
    IOExtender* ioExtender_ = nullptr;
};

/**
 * InputBit class represents a GPIO pin configured as an input. Works for both pins on the
 * ESP32 and pins on an IO extender (like PCA9557). Provides methods to get the pin state.
 */
class InputBit {
   public:
    /**
     * Constructor for when you want to specify a name for the input bit. This name
     * is used for logging and debugging purposes. The bit is configured as an input bit in
     * the constructor.
     * @param num The GPIO pin number.
     * @param name The name of the output bit (for logging).
     * @param ioExtender_ Pointer to an IOExtender object, if using one.
     */
    InputBit(GPIONum num, std::string bit_name, IOExtender* ioExtender_ = nullptr);

    /**
     * Constructor for when don't want to specify a name for the output bit.
     * @param num The GPIO pin number.
     * @param ioExtender_ Pointer to an IOExtender object, if using one.
     */
    InputBit(GPIONum num, IOExtender* ioExtender_ = nullptr);

    ~InputBit();

    /**
     * Returns the current state of the output bit.
     *  @return true if the pin is HIGH, false if it is LOW.
     */
    bool get() const;

   private:
    GPIONum pin_;
    std::string bit_name_;
    GPIOInput* gpioInput_ = nullptr;  // Managed by the destructor
    IOExtender* ioExtender_;
};

/**
 * There are multiple timers built into the ESP32. The number is specified by LEDC_TIMER_MAX,
 * which is 4 for the ES32S3. Timers can be shared between GPIO outputs, but since the timer
 * sets the frequency of the output all of the GPIO outputs will share that frequency. If
 * specific timer not specified then LEDC_TIMER_0 will be used. And if the frequency is not
 * specified then the default of kDefaultFrequency=1000hz will be used.
 */
class PWMTimer {
    const static uint32_t kDefaultFrequency = 1000;

   public:
    /**
     * Provides a timer that isn't already being used. If none available then returns nullptr.
     * Intended to be easier to use than the constructor.
     * @param freq_hz the initial frequency for the timer
     * @return pointer to a PWMTimer, or nullptr if none are available
     */
    static PWMTimer* getAvailableTimer(const uint32_t freq_hz = kDefaultFrequency);

    /**
     * For if one wants a specific timer, one that already might be in use.
     * @param timer_num the number of the timer to use
     * @param freq_hz the initial frequency for the timer
     * @return pointer to a PWMTimer
     */
    static PWMTimer* getTimer(const ledc_timer_t timer_num, const uint32_t freq_hz = kDefaultFrequency);

    /**
     * Releases reference to timer. Will destruct timer if there are no more references to it
     */
    void doneWithTimer();

    /**
     * Sets frequency of timer to the new value. All output PWM bits that use the
     * timer will be affected by this change. Note: changing the frequency will
     * also proportionally change the duty of the output.
     */
    void setFrequency(const uint32_t freq_hz);

    /**
     * Returns which timer is used. Will be LEDC_TIMER_0 to LEDC_TIMER_3
     */
    ledc_timer_t getTimer() const {
        return timer_num_;
    }

    /**
     * Returns speed mode used for the timer
     */
    ledc_mode_t getSpeedMode() const {
        return speed_mode_;
    }

    // Since using duty_resolution of LEDC_TIMER_12_BIT the duty cycle can range from 0 to 4096
    static const int MAX_DUTY = 4096;

   private:
    /**
     * Creates the timer. Private since should use get_available_timer() or get_timer() instead
     */
    PWMTimer(const ledc_timer_t timer_num, const uint32_t freq_hz);

    /**
     * Releases the timer. Private since should use done_with_timer() when done with a timer.
     */
    ~PWMTimer();


    // Disallow access to copy and assignment constructors since don't want constructor called inadvertantly
    PWMTimer(const PWMTimer& obj) = delete;
    PWMTimer& operator=(const PWMTimer& obj) = delete;

    const ledc_timer_t timer_num_;
    const ledc_mode_t speed_mode_;
    uint32_t freq_hz_;
    uint32_t num_references_;
};

/**
 * An output bit that instead of being binary, outputs a Pulse Width Modulation (PWM)
 * signal. The power and the frequency of the output can both be controlled after
 * the OutputPWM object has been constructed.
 *
 * Esperessif calls this the LED output because it is often used for controlling the
 * intensity of an LED. But it can also be used for other purpses such as controlling
 * a buzzer or a motor. The Esperessif documentation on this functionality is at
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html
 *
 * The LEDC channels are divided into two groups of 8 channels each. One group of LEDC
 * channels operates in high speed mode. This mode is implemented in hardware and offers
 * automatic and glitch-free changing of the PWM duty cycle. The other group of channels
 * operate in low speed mode, the PWM duty cycle must be changed by the driver in software.
 * Each group of channels is also able to use different clock sources.
 *
 * Number of LEDC channels available is specified by LEDC_CHANNEL_MAX, which is 8 for ESP32S3.
 * If the channel is not specified then the default of LEDC_CHANNEL_0 will be used.
 *
 * The LEDC channel controls the duty cycle for the PWM output.
 * Unlike Timers, LEDC channels are for a specific GPIO bit and therefore cannot be shared.
 */
class OutputPWM {
   public:
   /**
    * LEDC Channels are not shared, Therefore for each GPIO bit need a unique channel. Ane there
    * are only a few available. And it is difficult to keep track of whether one is still being
    * used or not. Therefore best to use get_available_channel() determine which channel to use
    * when calling OutputPWM() constructor.
    * @return the available channel to use, or LEDC_CHANNEL_MAX if there isn't one available
    */
   static ledc_channel_t get_available_channel(void);

   /**
    * Creates an output bit that can output a PWM signal of specific duty cycle and frequency.
    * Uses get_available_channel() to determine channel number to use.
    */
   OutputPWM(gpio_num_t gpio_num);

    /**
     * Creates an output bit that can output a PWM signal of specific duty cycle and frequency.
     * Recommend that the other constructor is used so that don't have to determine what
     * channel number to use.
     * @param gpio_num The number of the GPIO bit to control
     * @param channel LEDC channel to use (LEDC_CHANNEL_0 - LEDC_CHANNEL_MAX-1). Default is
     * LEDC_CHANNEL_0.
     */
    OutputPWM(gpio_num_t gpio_num, ledc_channel_t channel);

    ~OutputPWM();

    /**
     * Sets the duty cycle (power) of the output.
     * @param percentage The duty cycle of the output as a 0.0 - 100.0 percentage.
     */
    void setDuty(const float percentage);

    /**
     * Sets the duty cycle (power) of the output.
     * @param duty The duty cycle of the output. Since the timer has been configured for 12-bit
     * output the duty value can be set to between 0 (no power) and 4096 (full power).
     */
    void setDutyValue(const uint32_t duty);

    /**
     * Sets frequency of timer to the new value. All output PWM bits that use the
     * timer will be affected by this change. Since changing frequency of timer will
     * also change the duty of the signal proportionally, after the frequency is changed
     * the duty is reset to its previous value.
     */
    void setFrequency(const uint32_t freq_hz);

   private:
    // Disallow access to copy & assignment constructors since don't want constructor called inadvertantly
    OutputPWM(const OutputPWM& obj) = delete;
    OutputPWM& operator=(const OutputPWM& obj) = delete;

    PWMTimer* timer_ptr_;
    const int gpio_num_;
    const ledc_channel_t channel_;
    const ledc_mode_t speed_mode_;
    uint32_t duty_;  // not the percentage, but instead the integer value
};

}  // namespace idfx
