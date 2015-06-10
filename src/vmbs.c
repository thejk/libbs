#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "libbs.h"
#include "extra_compiler_stuff.h"

#if HAVE_GETOPT_LONG
# include <getopt.h>
#endif

static struct {
    const char* serial;
    uint16_t leds;
    bool quit;
} glob;

static bool handle_args(int argc, char** argv, int* exitcode);
static bool init(bs_device_t* dev);
static bool run(bs_device_t* dev);
static void clear(bs_device_t* dev);

static bool run_capture(bs_device_t* dev, bs_color_t* table,
                        const bs_color_t* blue_table,
                        const bs_color_t* normal_table);

int main(int argc, char** argv) {
    int exitcode;
    bs_device_t* dev;
    bs_error_t error;
    if (!handle_args(argc, argv, &exitcode)) {
        return exitcode;
    }
        if (glob.serial) {
        dev = bs_open_matching_serial(glob.serial, &error);
    } else {
        dev = bs_open_first(&error);
    }
    if (!dev) {
        if (error == BS_NO_ERROR) {
            fputs("Unable to find a BlinkStick\n", stderr);
        } else {
            fprintf(stderr, "Error opening BlinkStick: %s\n",
                    bs_error_str(error));
        }
        return EXIT_FAILURE;
    }

    if (!init(dev)) {
        bs_close(dev);
        return EXIT_FAILURE;
    }
    exitcode = run(dev) ? EXIT_SUCCESS : EXIT_FAILURE;
    clear(dev);
    bs_close(dev);
    return exitcode;
}

static void print_usage() {
    fputs("Usage: `vmbs [OPTIONS...]`\n", stdout);
    fputs("Display a volume meter on your BlinkStick\n", stdout);
    fputs("\n", stdout);
    fputs("Options:\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -s, --serial=SERIAL    ", stdout);
#else
    fputs("  -s SERIAL              ", stdout);
#endif
    fputs("work on the BlinkStick with this SERIAL\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -V, --version          ", stdout);
#else
    fputs("  -V                     ", stdout);
#endif
    fputs("display version and exit\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -h, --help             ", stdout);
#else
    fputs("  -h                     ", stdout);
#endif
    fputs("display this text and exit\n", stdout);
    fputs("\n", stdout);
}

bool handle_args(int argc, char** argv, int* exitcode) {
    const char* shortopts = "Vhs:";
    bool error = false, usage = false, version = false;
#if HAVE_GETOPT_LONG
    static const struct option longopts[] = {
        { "version", no_argument,       NULL, 'V' },
        { "help",    no_argument,       NULL, 'h' },
        { "serial",  required_argument, NULL, 's' },
        { NULL,      0,                 NULL,  0  }
    };
#endif
    while (true) {
        int c;
#if HAVE_GETOPT_LONG
        int index;
        c = getopt_long(argc, argv, shortopts, longopts, &index);
#else
        c = getopt(argc, argv, shortopts);
#endif
        if (c == -1) break;
        switch (c) {
        case 'V':
            version = true;
            break;
        case 'h':
            usage = true;
            break;
        case 's':
            glob.serial = optarg;
            break;
        case '?':
            error = true;
            break;
        }
    }
    if (optind < argc) {
        fputs("No arguments expected\n", stderr);
        error = true;
    }
    if (usage) {
        print_usage();
        *exitcode = error ? EXIT_FAILURE : EXIT_SUCCESS;
        return false;
    }
    if (error) {
#if HAVE_GETOPT_LONG
        fputs("Try `bs --help` for usage\n", stderr);
#else
        fputs("Try `bs -h` for usage\n", stderr);
#endif
        *exitcode = EXIT_FAILURE;
        return false;
    }
    if (version) {
        fputs("bs " VERSION " written by Joel Klinghed\n", stdout);
        *exitcode = EXIT_SUCCESS;
        return false;
    }
    return true;
}

static const bs_color_t black = { 0, 0, 0 };

bool init(bs_device_t* dev) {
    glob.leds = bs_get_max_leds(dev);
    if (glob.leds == 0) {
        glob.leds = 1;
        bs_set_mode(dev, BS_MODE_REPEAT);
    }

    if (glob.leds == 1) {
        return bs_set(dev, black);
    }
    switch (bs_get_mode(dev)) {
    case BS_MODE_REPEAT:
        if (!bs_set(dev, black)) return false;
        return bs_set_mode(dev, BS_MODE_MULTI);
    case BS_MODE_NORMAL:
    case BS_MODE_INVERSE:
        // Should never happen
        glob.leds = 1;
        return bs_set(dev, black);
    case BS_MODE_MULTI: {
        bs_color_t* colors = calloc(glob.leds, sizeof(bs_color_t));
        bool ret = bs_set_many(dev, glob.leds, colors);
        free(colors);
        return ret;
    }
    }
    return false;
}

static const bs_color_t red = { 0xff, 0x00, 0x00 };
static const bs_color_t yellow = { 0xff, 0x80, 0x00 };
static const bs_color_t green = { 0x00, 0xfc, 0x00 };
static const bs_color_t blue = { 0x00, 0x00, 0xff };

static void scale(bs_color_t* clr, unsigned char value) {
    if (value == 255) return;
    if (value == 0) {
        memset(clr, 0, sizeof(bs_color_t));
        return;
    } else {
        if (clr->red) {
            clr->red = ((unsigned int)clr->red * value + 255) >> 8;
        }
        if (clr->green) {
            clr->green = ((unsigned int)clr->green * value + 255) >> 8;
        }
        if (clr->blue) {
            clr->blue = ((unsigned int)clr->blue * value + 255) >> 8;
        }
    }
}

static void merge(bs_color_t* clr, bs_color_t left, double d_value,
                  bs_color_t right) {
    if (d_value <= 0.0) {
        *clr = left;
    } else if (d_value >= 1.0) {
        *clr = right;
    } else {
        const unsigned int value = 256 * d_value;
        const unsigned int inv_value = 256 - value;
        clr->red = ((unsigned int)left.red * value +
                    (unsigned int)right.red * inv_value) >> 8;
        clr->green = ((unsigned int)left.green * value +
                      (unsigned int)right.green * inv_value) >> 8;
        clr->blue = ((unsigned int)left.blue * value +
                     (unsigned int)right.blue * inv_value) >> 8;

    }
}

static void calc_blue(bs_color_t* table) {
    const double blue_part = glob.leds / 8.0;
    const size_t num = ceil(blue_part);
    size_t i;
    memset(table + num, 0, sizeof(bs_color_t) * (glob.leds - num));
    for (i = 0; i < num; i++) {
        table[i] = blue;
    }
    if (num > 0) {
        scale(table + (num - 1), 255 * (1.0 - num + blue_part));
    }
}

static void calc_normal(bs_color_t* table) {
    const double green_end = (glob.leds * 5.0) / 8.0;
    const double yellow_end = (glob.leds * 7.0) / 8.0;
    const size_t low_green = floor(green_end), high_green = ceil(green_end);
    const size_t low_yellow = floor(yellow_end), high_yellow = ceil(yellow_end);
    size_t i;
    for (i = 0; i < glob.leds; i++) {
        if (i < low_green) {
            table[i] = green;
        } else if (i >= high_green && i < low_yellow) {
            table[i] = yellow;
        } else if (i >= high_yellow) {
            table[i] = red;
        } else if (i == low_green && high_green == low_yellow) {
            merge(table + i, green, 1.0 - green_end + low_green, yellow);
        } else {
            merge(table + i, yellow, 1.0 - yellow_end + low_yellow, red);
        }
    }
}

static bool set_value(bs_device_t* dev,
                      unsigned char value, bs_color_t* table,
                      const bs_color_t* blue_table,
                      const bs_color_t* normal_table) {
    if (glob.leds == 1) {
        if (value == 0) {
            *table = blue;
        } else {
            *table = green;
            scale(table, value);
        }
        return bs_set(dev, *table);
    }
    if (value == 0) {
        memcpy(table, blue_table, sizeof(bs_color_t) * glob.leds);
    } else {
        double fill = (glob.leds * value) / 255.0;
        size_t high = ceil(fill);
        memcpy(table, normal_table, sizeof(bs_color_t) * high);
        memset(table + high, 0, sizeof(bs_color_t) * (glob.leds - high));
        if (high > 0) {
            scale(table + high - 1, 255 * (1.0 - high + fill));
        }
    }
    return bs_set_many(dev, glob.leds, table);
}

static void do_quit(int signum UNUSED) {
    glob.quit = true;
}

bool run(bs_device_t* dev) {
    bs_color_t* table = calloc(glob.leds * 3, sizeof(bs_color_t));
    bs_color_t* blue_table = table + glob.leds;
    bs_color_t* normal_table = blue_table + glob.leds;
    bool ret;
    calc_blue(blue_table);
    calc_normal(normal_table);
    signal(SIGINT, do_quit);
    signal(SIGTERM, do_quit);

    ret = run_capture(dev, table, blue_table, normal_table);

    free(table);
    return ret;
}

void clear(bs_device_t* dev) {
    if (glob.leds == 1) {
        bs_set(dev, black);
    } else {
        bs_color_t* colors = calloc(glob.leds, sizeof(bs_color_t));
        bs_set_many(dev, glob.leds, colors);
        free(colors);
    }
}

bool run_capture(bs_device_t* dev, bs_color_t* table,
                 const bs_color_t* blue_table,
                 const bs_color_t* normal_table) {
    /* Fallback, just slowly go from 0 to max and back again */
    unsigned char value = 1;
    bool down = true;
    while (!glob.quit) {
        if (value == 255 || value == 0) {
            down = !down;
        }
        if (down) {
            value--;
        } else {
            value++;
        }
        if (!set_value(dev, value, table, blue_table, normal_table)) {
            return false;
        }
        sleep(1);
    }
    return true;
}
