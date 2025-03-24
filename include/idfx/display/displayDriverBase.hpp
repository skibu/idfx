/**
 * Abstract base class for a display driver that can be used with LVGL
 *
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"

class DisplayDriverBase {
   public:
    DisplayDriverBase(int width, int height);

    virtual ~DisplayDriverBase() = default;

   protected:
    /**
     * Initialize the display driver
     */
    virtual void init() = 0;

    /**
     * Get the width of the display in pixels
     * @return The width of the display in pixels
     */
    int width() {
        return width_;
    };

    /**
     * Get the height of the display in pixels
     * @return The height of the display in pixels
     */
    int height() {
        return height_;
    };

    /**
     * Flush the display buffer to the display
     */
    virtual void flush() = 0;

    /**
     * Get the display
     * @return The display
     */
    virtual lv_disp_t *getDisp() = 0;

    /**
     * Get the display buffer size
     * @return The display buffer size
     */
    virtual size_t getDispBufSize() = 0;

   private:
    int width_;
    int height_;
};