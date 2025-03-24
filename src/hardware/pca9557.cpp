/*
 * SPDX-License-Identifier: MIT
 */

#include "idfx/hardware/pca9557.hpp"

#include "idfx/utils/log.hpp"

using namespace idfx;

PCA9557::PCA9557(I2CMaster &master, I2CAddress address)
    : master(master), address(address) {
    DEBUG("Creating PCA9557 I2C based IO expander object");

    master.sync_write(address, {POLARITY_REGISTER});
    std::vector<uint8_t> result2 = master.sync_read(address, 1);
    VERBOSE("Polarity register initially was: %X", result2[0]);
    // Clear the polarity since the hardware default is a very odd 0xF0
    uint8_t CLEAR = 0x00;
    std::vector<uint8_t> result = master.sync_transfer(address, {POLARITY_REGISTER, CLEAR}, 1);
    if (result[0] != CLEAR) {
        ERROR("Failed to set polarity register to 0x00\n");
    }
}

PCA9557::~PCA9557() {
    VERBOSE("PCA9557 object is being destroyed");
}

void PCA9557::configAsOutput(int io_bit) {
    DEBUG("Configuring PCA9557 IO bit %d as output", io_bit);

    // Read the current config. First need to write to the register address (I think)
    std::vector<uint8_t> result = master.sync_transfer(address, {CONFIG_REGISTER}, 1);
    VERBOSE("Originally config was: 0x%02X", result[0]);

    uint8_t new_value = result[0] & ~(1 << io_bit);
    VERBOSE("Config will be set to: 0x%02X", new_value);
    result = master.sync_transfer(address, {CONFIG_REGISTER, new_value}, 1);
    VERBOSE("After setting it the config returned is: 0x%02X", result[0]);
}

void PCA9557::configAsInput(int io_bit) {
    DEBUG("Configuring PCA9557 IO bit %d as input", io_bit);

    // Read the current config. First need to write to the register address (I think)
    std::vector<uint8_t> result = master.sync_transfer(address, {CONFIG_REGISTER}, 1);
    VERBOSE("Originally config is: 0x%02X", result[0]);

    uint8_t new_value = result[0] | (1 << io_bit);
    VERBOSE("Config will be set to: 0x%02X", new_value);
    result = master.sync_transfer(address, {CONFIG_REGISTER, new_value}, 1);
    VERBOSE("After setting it the config returned is: 0x%02X\n", result[0]);
}

void PCA9557::setBit(int io_bit, bool on) {
    DEBUG("Setting IO bit %d on the pca9557 to %d", io_bit, on);

    // Get current state
    std::vector<uint8_t> result = master.sync_transfer(address, {CURRENT_VALUES_REGISTER}, 1);
    VERBOSE("Initial value was: 0x%02X", result[0]);

    // Set the specified bit to the specified value
    volatile uint8_t new_value = result[0];
    if (on) {
        new_value |= (1 << io_bit);
    } else {
        new_value &= ~(1 << io_bit);
    }
    VERBOSE("New value will be: 0x%02X", new_value);
    result = master.sync_transfer(address, {OUTPUT_PORT_REGISTER, new_value}, 1);
    DEBUG("Set PCA9557 expander output IO bit %d to %s the value returned is: 0x%02X",
         io_bit, on ? "on" : "off", result[0]);
}

uint8_t PCA9557::getBit(int io_bit) {
    DEBUG("Getting the current value of IO bit %d of the PCA9557", io_bit);
    std::vector<uint8_t> result = master.sync_transfer(address, {CURRENT_VALUES_REGISTER}, 1);
    bool bit_set = (result[0] & (1 << io_bit)) != 0;
    DEBUG("On PCA9557 expander bit %d is currently %d\n", io_bit, bit_set);
    return bit_set;
}