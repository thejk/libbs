#ifndef LIBBS_H
#define LIBBS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "compiler_stuff.h"

typedef struct bs_device_t bs_device_t;

typedef struct bs_color_t {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} bs_color_t;

/**
 * Init libbs.
 * You don't have to call this method, but if you do you must call bs_shutdown()
 * after all devices have been closed. Calling this method is a bit more
 * effective if you expect any of the bs_open_* calls to fail to find a
 * BlinkStick. Also useful if you want to distinguish from library errors and no
 * BlinkStick found.
 * no-op if called more than once but it's undefined to call bs_shutdown()
 * more than once. Calling bs_init(), bs_shutdown() and then bs_init() again
 * is OK tho.
 * @return false if there was an error initializing the library
 */
BS_API bool bs_init(void);

/**
 * Shutdown libbs after a call to bs_init. Make sure all open devices are
 * closed before calling.
 */
BS_API void bs_shutdown(void);

/**
 * Open first BlinkStick found.
 * Remember to close returned device.
 * @return device or NULL in case of error
 */
BS_API bs_device_t* bs_open_first(void) MALLOC;
/**
 * Open BlinkStick with matching serial if found.
 * Remember to close returned device.
 * @param serial serial to search for, may not be NULL
 * @return device or NULL in case of error
 */
BS_API bs_device_t* bs_open_matching_serial(const char* serial) NONULL MALLOC;
/**
 * Open all BlinkStick devices found.
 * Remember to close each individual device when done and then free the array
 * itself.
 * @param max maximum number of devices to return, if 0 is given a default is
 *            used (currently 12)
 * @return NULL-terminated array of open devices or NULL in case of error
 */
BS_API bs_device_t** bs_open_all(size_t max) MALLOC;

/**
 * Close open device, calling twice on the same device is undefined.
 * Calling with NULL as argument is a no-op.
 * @param device to close, may be NULL
 */
BS_API void bs_close(bs_device_t* device);

/**
 * @param device to get serial from, may not be NULL
 * @return a copy of the serial for the device, or NULL in case of error
 */
BS_API char* bs_serial(bs_device_t* device) NONULL MALLOC;

/**
 * @param device to check, may not be NULL
 * @return true if device seems to be working
 */
BS_API bool bs_good(bs_device_t* device) NONULL;

/**
 * Set current color
 * @param device device to change color on, may not be NULL
 * @param color color to set
 * @return false if there was an error
 */
BS_API bool bs_set(bs_device_t* device, bs_color_t color) NONULL;

/**
 * Get current color
 * @param device device to read color from, may not be NULL
 * @param color pointer to receive current color, may not be NULL
 * @return false if there was an error
 */
BS_API bool bs_get(bs_device_t* device, bs_color_t* color) NONULL;

/**
 * Set color of indexed led on a pro stick
 * @param device device to change color on, may not be NULL
 * @param index index to set
 * @param color color to set
 * @return false if there was an error
 */
BS_API bool bs_set_pro(bs_device_t* device, uint8_t index,
                       bs_color_t color) NONULL;

/**
 * Get color of indexed led on a pro stick
 * @param device device to read color from, may not be NULL
 * @param index index to get
 * @param color pointer to receive current color, may not be NULL
 * @return false if there was an error
 */
BS_API bool bs_get_pro(bs_device_t* device, uint8_t index,
                       bs_color_t* color) NONULL;

/**
 * Set color of many indexed led at the same time
 * @param device device to change colors on, may not be NULL
 * @param count number of leds to change, always 0-count
 *        please note that at this time blinksticks are limited to either
 *        setting 8, 16, 32 or 64 leds at the same time, a count of five
 *        will still set eight leds, just use black for the last three
 * @param color color of each led, may not be NULL
 * @return false if there was an error
 */
BS_API bool bs_set_many(bs_device_t* device, uint8_t count,
                        const bs_color_t* color) NONULL;

/**
 * Get color of many indexed led at the same time
 * @param device device to change colors on, may not be NULL
 * @param count number of leds to query, always 0-count
 * @param color receive color of each led, may not be NULL
 * @return false if there was an error
 */
BS_API bool bs_get_many(bs_device_t* device, uint8_t count,
                        bs_color_t* color) NONULL;

#undef BS_API

#endif /* LIBBS_H */
