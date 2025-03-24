/*
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>

namespace idfx {

class IOExtender {
   public:
    /* Sets the specified config bit to 0, which indicates it is an output bit. */
    virtual void configAsOutput(int io_bit) = 0;

    /* Sets the specified config bit to 1, which indicates it is an input bit. */
    virtual void configAsInput(int io_bit) = 0;

    /* Sets the specified bit to on or off. */
    virtual void setBit(int io_bit, bool on) = 0;

    /* Works for both input and output bits */
    virtual uint8_t getBit(int io_bit) = 0;
};

}  // namespace idfx