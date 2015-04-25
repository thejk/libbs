#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include "libbs.h"

int main(int argc, char** argv) {
    bs_device_t** devs = bs_open_all(0);
    if (devs == NULL) {
        fputs("Error listing BlinkStick devices\n", stderr);
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
