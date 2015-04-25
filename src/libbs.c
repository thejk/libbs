#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include "libbs.h"

#include <libusb.h>

#include <stdio.h>

struct {
    libusb_context* ctx;
    bool forced;
    long devices;
} glob;

static bool init_glob(void) {
    if (glob.ctx == NULL) {
        if (libusb_init(&glob.ctx) != 0) {
            return false;
        }
        assert(glob.ctx);
        libusb_set_debug(glob.ctx, 3);
    }
    return true;
}

static void deinit_glob(void) {
    if (!glob.forced && glob.devices == 0 && glob.ctx) {
        libusb_exit(glob.ctx);
        glob.ctx = NULL;
    }
}

struct bs_device_t {
    libusb_device_handle* handle;
    char* serial;
};

bool bs_init(void) {
    if (glob.forced) return true;
    if (!init_glob()) return false;
    glob.forced = true;
    return true;
}

void bs_shutdown(void) {
    assert(glob.devices == 0);
    assert(glob.forced);
    glob.forced = false;
    deinit_glob();
}

static bs_device_t* bs_open(libusb_device* device, const char* match_serial) {
    libusb_device_handle* handle;
    bs_device_t* dev;
    struct libusb_device_descriptor desc;
    char tmp[256];
    int len;
    char* end;
    if (libusb_get_device_descriptor(device, &desc)) return NULL;
    if (desc.idVendor != 0x20a0 || desc.idProduct != 0x41e5) return NULL;
    if (libusb_open(device, &handle) != 0) return NULL;
    len = libusb_get_string_descriptor_ascii(
            handle, desc.iSerialNumber, (uint8_t*)tmp, sizeof(tmp) - 1);
    if (len <= 3) {
        libusb_close(handle);
        return NULL;
    }
    tmp[len] = '\0';
    if (memcmp(tmp, "BS", 2) != 0 ||
        (match_serial && strcmp(tmp, match_serial) != 0)) {
        libusb_close(handle);
        return NULL;
    }
    dev = malloc(sizeof(bs_device_t));
    dev->handle = handle;
    dev->serial = malloc(len + 1);
    memcpy(dev->serial, tmp, len + 1);
    glob.devices++;
    return dev;
}

bs_device_t* bs_open_first(void) {
    size_t i;
    ssize_t count;
    libusb_device** devices;
    bs_device_t* dev = NULL;
    if (!init_glob()) return NULL;
    count = libusb_get_device_list(glob.ctx, &devices);
    if (count < 0) return NULL;
    for (i = 0; i < (size_t)count; i++) {
        dev = bs_open(devices[i], NULL);
        if (dev) break;
    }
    libusb_free_device_list(devices, 1);
    return dev;
}

bs_device_t* bs_open_matching_serial(const char* serial) {
    size_t i;
    ssize_t count;
    libusb_device** devices;
    bs_device_t* dev = NULL;
    if (!serial) {
        assert(false);
        return NULL;
    }
    if (!init_glob()) return NULL;
    count = libusb_get_device_list(glob.ctx, &devices);
    if (count < 0) return NULL;
    for (i = 0; i < (size_t)count; i++) {
        dev = bs_open(devices[i], serial);
        if (dev) break;
    }
    libusb_free_device_list(devices, 1);
    return dev;
}

bs_device_t** bs_open_all(size_t max) {
    size_t i, open = 0, alloc;
    ssize_t count;
    libusb_device** devices;
    bs_device_t** dev = NULL;
    if (!init_glob()) return NULL;
    if (max == 0) max = 12;
    count = libusb_get_device_list(glob.ctx, &devices);
    if (count < 0) return NULL;
    alloc = (size_t)count;
    if (alloc > max) alloc = max;
    dev = calloc(sizeof(bs_device_t*), alloc + 1);
    for (i = 0; i < (size_t)count; i++) {
        bs_device_t* d = bs_open(devices[i], NULL);
        if (d) {
            assert(open < alloc);
            dev[open++] = d;
        }
    }
    libusb_free_device_list(devices, 1);
    dev[open] = NULL;
    return dev;
}

void bs_close(bs_device_t* device) {
    if (device == NULL) return;
    libusb_close(device->handle);
    free(device->serial);
    free(device);
    assert(glob.devices > 0);
    glob.devices--;
    deinit_glob();
}

char* bs_serial(bs_device_t* device) {
    if (device == NULL) {
        assert(false);
        return NULL;
    }
    return strdup(device->serial);
}

bool bs_good(bs_device_t* device) {
    bs_color_t clr;
    if (device == NULL) {
        assert(false);
        return false;
    }
    /* TODO: Make a more effective version? */
    return bs_get(device, &clr);
}

bool bs_set(bs_device_t* device, bs_color_t color) {
    return bs_set_pro(device, 0, color);
}

bool bs_get(bs_device_t* device, bs_color_t* color) {
    return bs_get_pro(device, 0, color);
}

static unsigned int TIMEOUT = 0;

static ssize_t bs_ctrl_transfer(bs_device_t* device, uint8_t request_type,
                                uint8_t request, uint16_t value, uint16_t index,
                                uint8_t* data, uint16_t length) {
    int ret = libusb_control_transfer(device->handle, request_type, request,
                                      value, index, data, length, TIMEOUT);
    if (ret >= 0) return ret;
    if (ret == LIBUSB_ERROR_NO_DEVICE) {
        bs_device_t* dev = bs_open_matching_serial(device->serial);
        if (dev != NULL) {
            libusb_close(device->handle);
            device->handle = dev->handle;
            free(dev->serial);
            free(dev);
            ret = libusb_control_transfer(device->handle, request_type, request,
                                          value, index, data, length, TIMEOUT);
        }
    }
    return ret;
}

bool bs_set_pro(bs_device_t* device, uint8_t index, bs_color_t color) {
    uint8_t data[6];
    if (device == NULL) {
        assert(false);
        return false;
    }
    if (index == 0) {
        data[0] = 0;
        data[1] = color.red;
        data[2] = color.green;
        data[3] = color.blue;
        return bs_ctrl_transfer(device, 0x20, 0x9, 1, 0, data, 4) == 4;
    } else {
        data[0] = 5;
        data[1] = 0;  /* Channel */
        data[2] = index;
        data[3] = color.red;
        data[4] = color.green;
        data[5] = color.blue;
        return bs_ctrl_transfer(device, 0x20, 0x9, 5, 0, data, 6) == 6;
    }
}

static uint8_t report_id(uint8_t count) {
    if (count <= 8) {
        return 6;
    } else if (count <= 16) {
        return 7;
    } else if (count <= 32) {
        return 8;
    }
    return 9;
}

static size_t min_size(uint8_t count) {
    if (count <= 8) {
        count = 8;
    } else if (count <= 16) {
        count = 16;
    } else if (count <= 32) {
        count = 32;
    } else {
        count = 64;
    }
    return 2 + count * 3;
}

bool bs_get_pro(bs_device_t* device, uint8_t index, bs_color_t* color) {
    if (device == NULL || color == NULL) {
        assert(false);
        return false;
    }
    if (index == 0) {
        uint8_t data[4];
        if (bs_ctrl_transfer(device, 0x80 | 0x20, 0x1, 1, 0,
                             data, sizeof(data)) != 4) {
            return false;
        }
        color->red = data[1];
        color->green = data[2];
        color->blue = data[3];
        return true;
    } else {
        uint8_t data[2 + 64 * 3];
        size_t size = min_size(index + 1);
        if (bs_ctrl_transfer(device, 0x80 | 0x20, 0x1, report_id(index + 1), 0,
                             data, size) < size) {
            return false;
        }
        color->red = data[2 + index * 3 + 1];
        color->green = data[2 + index * 3 + 0];
        color->blue = data[2 + index * 3 + 2];
        return true;
    }
}

bool bs_set_many(bs_device_t* device, uint8_t count, const bs_color_t* color) {
    size_t len;
    uint8_t data[2 + 64 * 3];
    uint8_t i;
    size_t o, size;
    if (count == 0) return true;
    if (count > 64) count = 64;
    if (device == NULL || color == NULL) {
        assert(false);
        return false;
    }
    data[0] = 0;
    data[1] = 0;  /* Channel */
    o = 2;
    for (i = 0; i < count; i++) {
        data[o++] = color[i].green;
        data[o++] = color[i].red;
        data[o++] = color[i].blue;
    }
    size = min_size(count);
    memset(data + o, 0, size - o);
    return bs_ctrl_transfer(device, 0x20, 0x9, report_id(count), 0, data, size)
        == size;
}

bool bs_get_many(bs_device_t* device, uint8_t count, bs_color_t* color) {
    size_t len;
    uint8_t data[2 + 64 * 3];
    uint8_t i;
    size_t o;
    if (count == 0) return true;
    if (device == NULL || color == NULL) {
        assert(false);
        return false;
    }
    if (count > 64) {
        memset(color + 64, 0, sizeof(bs_color_t) * (count - 64));
        count = 64;
    }
    o = min_size(count);
    if (bs_ctrl_transfer(device, 0x20, 0x9, report_id(count), 0, data, o) < o) {
        return false;
    }
    i = count;
    o = 2 + count * 3;
    while (i > 0) {
        i--;
        color[i].blue = data[--o];
        color[i].red = data[--o];
        color[i].green = data[--o];
    }
    assert(o == 2);
    return true;
}
