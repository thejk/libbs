#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include "libbs.h"

#include <libusb.h>

#include <stdio.h>

static bs_error_t error_from_libusb(ssize_t err);

static struct {
    libusb_context* ctx;
    bool forced;
    long devices;
} glob;

static bool init_glob(bs_error_t* error) {
    if (glob.ctx == NULL) {
        int ret = libusb_init(&glob.ctx);
        if (ret != 0) {
            if (error) *error = error_from_libusb(ret);
            return false;
        }
        assert(glob.ctx);
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
    bs_error_t last_error;
};

bool bs_init(bs_error_t* error) {
    if (error) *error = BS_NO_ERROR;
    if (glob.forced) return true;
    if (!init_glob(error)) return false;
    glob.forced = true;
    return true;
}

void bs_shutdown(void) {
    assert(glob.devices == 0);
    assert(glob.forced);
    glob.forced = false;
    deinit_glob();
}

static bs_device_t* bs_open(libusb_device* device, const char* match_serial,
                            bs_error_t* error) NONULL_ARGS(1) MALLOC;

bs_device_t* bs_open(libusb_device* device, const char* match_serial,
                     bs_error_t* error) {
    libusb_device_handle* handle;
    bs_device_t* dev;
    struct libusb_device_descriptor desc;
    char tmp[256];
    int ret, len;
    char* end;
    ret = libusb_get_device_descriptor(device, &desc);
    if (ret) {
        if (error) *error = error_from_libusb(ret);
        return NULL;
    }
    if (desc.idVendor != 0x20a0 || desc.idProduct != 0x41e5) return NULL;
    do {
        ret = libusb_open(device, &handle);
    } while (ret == LIBUSB_ERROR_INTERRUPTED);
    if (ret) {
        if (error) *error = error_from_libusb(ret);
        return NULL;
    }
    do {
        len = libusb_get_string_descriptor_ascii(
                handle, desc.iSerialNumber, (uint8_t*)tmp, sizeof(tmp) - 1);
    } while (len == LIBUSB_ERROR_INTERRUPTED);
    if (len <= 3) {
        if (len < 0 && error) *error = error_from_libusb(len);
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
    dev->last_error = BS_NO_ERROR;
    glob.devices++;
    return dev;
}

bs_device_t* bs_open_first(bs_error_t* error) {
    size_t i;
    ssize_t count;
    libusb_device** devices;
    bs_device_t* dev = NULL;
    if (!init_glob(error)) return NULL;
    count = libusb_get_device_list(glob.ctx, &devices);
    if (count < 0) {
        if (error) *error = error_from_libusb(count);
        return NULL;
    }
    if (error) *error = BS_NO_ERROR;
    for (i = 0; i < (size_t)count; i++) {
        dev = bs_open(devices[i], NULL, error);
        if (dev) break;
    }
    libusb_free_device_list(devices, 1);
    return dev;
}

bs_device_t* bs_open_matching_serial(const char* serial, bs_error_t* error) {
    size_t i;
    ssize_t count;
    libusb_device** devices;
    bs_device_t* dev = NULL;
    if (!serial) {
        assert(false);
        if (error) *error = BS_ERROR_INVALID_PARAM;
        return NULL;
    }
    if (!init_glob(error)) return NULL;
    count = libusb_get_device_list(glob.ctx, &devices);
    if (count < 0) {
        if (error) *error = error_from_libusb(count);
        return NULL;
    }
    if (error) *error = BS_NO_ERROR;
    for (i = 0; i < (size_t)count; i++) {
        dev = bs_open(devices[i], serial, error);
        if (dev) break;
    }
    libusb_free_device_list(devices, 1);
    return dev;
}

bs_device_t** bs_open_all(size_t max, bs_error_t* error) {
    size_t i, open = 0, alloc;
    ssize_t count;
    libusb_device** devices;
    bs_device_t** dev = NULL;
    if (!init_glob(error)) return NULL;
    if (max == 0) max = 12;
    count = libusb_get_device_list(glob.ctx, &devices);
    if (count < 0) {
        if (error) *error = error_from_libusb(count);
        return NULL;
    }
    if (error) *error = BS_NO_ERROR;
    alloc = (size_t)count;
    if (alloc > max) alloc = max;
    dev = calloc(sizeof(bs_device_t*), alloc + 1);
    for (i = 0; i < (size_t)count; i++) {
        bs_device_t* d = bs_open(devices[i], NULL, NULL);
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

bs_error_t bs_error(bs_device_t* device) {
    if (device == NULL) {
        assert(false);
        return BS_ERROR_INVALID_PARAM;
    }
    return device->last_error;
}

bool bs_set(bs_device_t* device, bs_color_t color) {
    return bs_set_pro(device, 0, color);
}

bool bs_get(bs_device_t* device, bs_color_t* color) {
    return bs_get_pro(device, 0, color);
}

static unsigned int TIMEOUT = 0;

static bool bs_ctrl_transfer(bs_device_t* device, uint8_t request_type,
                             uint8_t request, uint16_t value, uint16_t index,
                             uint8_t* data, uint16_t length) NONULL;

bool bs_ctrl_transfer(bs_device_t* device, uint8_t request_type,
                      uint8_t request, uint16_t value, uint16_t index,
                      uint8_t* data, uint16_t length) {
    int ret;
    do {
        ret = libusb_control_transfer(device->handle, request_type, request,
                                      value, index, data, length, TIMEOUT);
    } while (ret == LIBUSB_ERROR_INTERRUPTED);
    if (ret == LIBUSB_ERROR_NO_DEVICE) {
        bs_device_t* dev = bs_open_matching_serial(device->serial, NULL);
        if (dev != NULL) {
            libusb_close(device->handle);
            device->handle = dev->handle;
            free(dev->serial);
            free(dev);
            do {
                ret = libusb_control_transfer(device->handle, request_type,
                                              request, value, index, data,
                                              length, TIMEOUT);
            } while (ret == LIBUSB_ERROR_INTERRUPTED);
        }
    }
    if (ret < 0) {
        device->last_error = error_from_libusb(ret);
        return false;
    }
    if ((uint16_t)ret != length) {
        device->last_error = BS_ERROR_COMM;
        return false;
    }
    return true;
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
        return bs_ctrl_transfer(device, 0x20, 0x9, 1, 0, data, 4);
    } else {
        data[0] = 5;
        data[1] = 0;  /* Channel */
        data[2] = index;
        data[3] = color.red;
        data[4] = color.green;
        data[5] = color.blue;
        return bs_ctrl_transfer(device, 0x20, 0x9, 5, 0, data, 6);
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
    if (device == NULL) {
        assert(false);
        return false;
    }
    if (color == NULL) {
        assert(false);
        device->last_error = BS_ERROR_INVALID_PARAM;
        return false;
    }
    if (index == 0) {
        uint8_t data[4];
        if (!bs_ctrl_transfer(device, 0x80 | 0x20, 0x1, 1, 0,
                              data, sizeof(data))) {
            return false;
        }
        color->red = data[1];
        color->green = data[2];
        color->blue = data[3];
        return true;
    } else {
        uint8_t data[2 + 64 * 3];
        if (!bs_ctrl_transfer(device, 0x80 | 0x20, 0x1, report_id(index + 1), 0,
                              data, min_size(index + 1))) {
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
    if (device == NULL) {
        assert(false);
        return false;
    }
    if (color == NULL) {
        assert(false);
        device->last_error = BS_ERROR_INVALID_PARAM;
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
    return bs_ctrl_transfer(device, 0x20, 0x9, report_id(count), 0, data, size);
}

bool bs_get_many(bs_device_t* device, uint8_t count, bs_color_t* color) {
    size_t len;
    uint8_t data[2 + 64 * 3];
    uint8_t i;
    size_t o;
    if (count == 0) return true;
    if (device == NULL) {
        assert(false);
        return false;
    }
    if (color == NULL) {
        assert(false);
        device->last_error = BS_ERROR_INVALID_PARAM;
        return false;
    }
    if (count > 64) {
        memset(color + 64, 0, sizeof(bs_color_t) * (count - 64));
        count = 64;
    }
    if (!bs_ctrl_transfer(device, 0x20, 0x9, report_id(count), 0, data,
                          min_size(count))) {
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

bs_error_t error_from_libusb(ssize_t err) {
    if (err >= 0) return BS_NO_ERROR;
    switch (err) {
    case LIBUSB_ERROR_IO:
        return BS_ERROR_IO;
    case LIBUSB_ERROR_INVALID_PARAM:
        return BS_ERROR_INVALID_PARAM;
    case LIBUSB_ERROR_ACCESS:
        return BS_ERROR_ACCESS;
    case LIBUSB_ERROR_NO_DEVICE:
    case LIBUSB_ERROR_NOT_FOUND:
        return BS_ERROR_DISCONNECTED;
    case LIBUSB_ERROR_BUSY:
        return BS_ERROR_BUSY;
    case LIBUSB_ERROR_TIMEOUT:
        return BS_ERROR_TIMEOUT;
    case LIBUSB_ERROR_OVERFLOW:
        return BS_ERROR_OVERFLOW;
    case LIBUSB_ERROR_PIPE:
        return BS_ERROR_PIPE;
    case LIBUSB_ERROR_NO_MEM:
        return BS_ERROR_NO_MEM;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return BS_ERROR_NOT_SUPPORTED;
    default:
        return BS_ERROR_UNKNOWN;
    }
}

const char* bs_error_str(bs_error_t error) {
    switch (error) {
    case BS_NO_ERROR:
        return "No error";
    case BS_ERROR_COMM:
        return "Communication error";
    case BS_ERROR_IO:
        return "Input/output error";
    case BS_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case BS_ERROR_ACCESS:
        return "Access denied";
    case BS_ERROR_DISCONNECTED:
        return "Device disconnected";
    case BS_ERROR_BUSY:
        return "Resource busy";
    case BS_ERROR_TIMEOUT:
        return "Operation timeout";
    case BS_ERROR_OVERFLOW:
        return "Overflow";
    case BS_ERROR_PIPE:
        return "Pipe error";
    case BS_ERROR_NO_MEM:
        return "Insufficient memory";
    case BS_ERROR_NOT_SUPPORTED:
        return "Operation not supported";
    case BS_ERROR_UNKNOWN:
        break;
    }
    return "Unknown error";
}
