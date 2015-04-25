#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include "extra_compiler_stuff.h"
#include "libbs.h"

int main(int argc UNUSED, char** argv UNUSED) {
    bs_error_t error;
    bs_device_t** devs = bs_open_all(0, &error);
    if (devs == NULL) {
        fprintf(stderr, "Error listing BlinkStick devices: %s\n",
                bs_error_str(error));
        return EXIT_FAILURE;
    }
    if (*devs) {
        bs_device_t** dev;
        for (dev = devs; *dev; dev++) {
            fputs(bs_serial(*dev), stdout);
            fputc('\n', stdout);
            bs_close(*dev);
        }
    } else {
        fputs("No working BlinkStick devices found\n", stdout);
    }
    free(devs);
    return EXIT_SUCCESS;
}
