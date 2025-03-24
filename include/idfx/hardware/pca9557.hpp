/* PCA9557 is IO extender that is communicated with via I2C.
 * Documentation on the PCA9557 is at https://www.ti.com/lit/ds/symlink/pca9557.pdf
 *
 * This software should also work for a TCA9534 chip, though that one also has
 * interrupts so the software would need to be extended to handle that. In fact,
 * inspiration for this code came from https://github.com/hideakitai/TCA9534/tree/master .
 *
 * The polarity feature of the PCA9557 is not made available in this software. The polarity
 * for each bit is initialized to be 0 for each bit, and no functions are provided to change it.
 *
 * SPDX-License-Identifier: MIT
 */

#include "i2c_cxx.hpp"
#include "idfx/hardware/io.hpp"

using namespace idfx;

class PCA9557 : public IOExtender {
   public:
    /* Constructs the PCA9557 object and initializes it so that polarity is set to 0x00 instead
       of the hardware default of 0xF0 */
    PCA9557(I2CMaster &master, I2CAddress address);

    ~PCA9557();

    /* Sets the specified config bit to 0, which indicates it is an output bit. */
    void configAsOutput(int io_bit);

    /* Sets the specified config bit to 1, which indicates it is an input bit. */
    void configAsInput(int io_bit);

    /* Sets the specified bit to on or off. */
    void setBit(int io_bit, bool on);

    /* Works for both input and output bits */
    uint8_t getBit(int io_bit);

   protected:
    I2CMaster &master;
    I2CAddress address;

    unsigned char CURRENT_VALUES_REGISTER = 0x00;
    unsigned char OUTPUT_PORT_REGISTER = 0x01;
    unsigned char POLARITY_REGISTER = 0x02;
    unsigned char CONFIG_REGISTER = 0x03;
};