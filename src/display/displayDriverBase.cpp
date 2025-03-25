/**
 * Base class for display drivers
 *
 * SPDX-License-Identifier: MIT
 */

#include "idfx/display/displayDriverBase.hpp"
#include "idfx/utils/log.hpp"

DisplayDriverBase::DisplayDriverBase(int width, int height) : width_(width), height_(height) {
    INFO("DisplayDriverBase constructor");
}