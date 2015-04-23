#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include "libbs.h"

int main(int argc, char** argv) {
    bs_device_t* dev = bs_open_first();
    bs_color_t clr;
    int i;
    if (!dev) {
        fputs("Unable to find a BlinkStick\n", stderr);
        return EXIT_FAILURE;
    }
    clr.red = 0;
    clr.green = 0;
    clr.blue = 0;
    bs_set_pro(dev, 0, 0, clr);
    bs_set_pro(dev, 0, 1, clr);
    bs_set_pro(dev, 0, 2, clr);
    bs_close(dev);
    return EXIT_SUCCESS;
}
