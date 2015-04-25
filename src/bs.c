#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libbs.h"

#if HAVE_GETOPT_LONG
# include <getopt.h>
#endif

static struct {
    const char* serial;
    bool verbose;
    bool reset;
    bool get_color;
    bool get_mode;
    bool set_mode;
    uint8_t mode;
    uint8_t index, count;
    bs_color_t color[64];
} glob;

static bool handle_args(int argc, char** argv, int* exitcode);
static bool reset(bs_device_t* dev);

int main(int argc, char** argv) {
    int exitcode;
    bs_device_t* dev;
    bool ret;
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
    if (glob.verbose && !glob.serial) {
        fprintf(stdout, "Found BlinkStick with serial: %s\n", bs_serial(dev));
    }
    if (glob.set_mode) {
        if (!bs_set_mode(dev, glob.mode)) {
            fprintf(stderr, "Error setting mode: %s\n",
                    bs_error_str(bs_error(dev)));
        }
    }
    if (glob.get_mode) {
        int mode = bs_get_mode(dev);
        if (mode >= 0) {
            fprintf(stdout, "Mode: %d ", mode);
            switch (mode) {
            case BS_MODE_NORMAL:
                fputs("Normal (single-led)\n", stdout);
                break;
            case BS_MODE_INVERSE:
                fputs("Inverse (single-led)\n", stdout);
                break;
            case BS_MODE_MULTI:
                fputs("Multi-led (WS2812)\n", stdout);
                break;
            case BS_MODE_REPEAT:
                fputs("Repeated multi-led (RGB-mirror)\n", stdout);
                break;
            default:
                fputs("???\n", stdout);
                break;
            }
        } else {
            fprintf(stderr, "Error getting mode: %s\n",
                    bs_error_str(bs_error(dev)));
        }
    }
    if (glob.reset) {
        if (!reset(dev)) {
            fprintf(stderr, "Error resetting: %s\n",
                    bs_error_str(bs_error(dev)));
        }
    }
    if (!glob.get_color) {
        if (glob.count == 0) {
            ret = true;
        } else if (glob.count == 1) {
            ret = bs_set_pro(dev, glob.index, glob.color[glob.index]);
        } else {
            memset(glob.color, 0, glob.index * sizeof(bs_color_t));
            ret = bs_set_many(dev, glob.index + glob.count, glob.color);
        }
    } else {
        if (glob.count == 1) {
            ret = bs_get_pro(dev, glob.index, glob.color + glob.index);
        } else {
            ret = bs_get_many(dev, glob.index + glob.count, glob.color);
        }
        if (ret) {
            uint8_t i;
            for (i = 0; i < glob.count; i++) {
                const bs_color_t* c = &glob.color[glob.index + i];
                fprintf(stdout, "%u: #%02x%02x%02x\n",
                        glob.index + i, c->red, c->green, c->blue);
            }
        }
    }
    if (!ret) {
        fprintf(stderr, "Error communicating with BlinkStick: %s\n",
                bs_error_str(bs_error(dev)));
    }
    bs_close(dev);
    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void print_usage() {
    fputs("Usage: `bs [OPTIONS...] COLOR [COLORS...]`\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("       `bs --get [OPTIONS...] [COUNT]`\n", stdout);
#else
    fputs("       `bs -g [OPTIONS...] [COUNT]`\n", stdout);
#endif
    fputs("Set or get one or more colors for your BlinkStick\n", stdout);
    fputs("Color can be either #RRGGBB or 0xRRGGBB\n", stdout);
    fputs("\n", stdout);
    fputs("Options:\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -g, --get              ", stdout);
#else
    fputs("  -g                     ", stdout);
#endif
    fputs("get color(s) instead of default set color(s)\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -m, --mode[=MODE]      ", stdout);
#else
    fputs("  -m [MODE]              ", stdout);
#endif
    fputs("set or get BlinkStick Pro mode\n", stdout);
    fputs("                         ", stdout);
    fputs("MODE can be 0 (Normal), 1 (Inverse), 2 (Multi) or 3 (Repeat).\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -r, --reset            ", stdout);
#else
    fputs("  -r                     ", stdout);
#endif
    fputs("reset BlinkStick to black before getting/setting colors\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -s, --serial=SERIAL    ", stdout);
#else
    fputs("  -s SERIAL              ", stdout);
#endif
    fputs("work on the BlinkStick with this SERIAL\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -i, --index=INDEX      ", stdout);
#else
    fputs("  -i INDEX               ", stdout);
#endif
    fputs("modify led at INDEX instead of the first one\n", stdout);
#if HAVE_GETOPT_LONG
    fputs("  -v, --verbose          ", stdout);
#else
    fputs("  -v                     ", stdout);
#endif
    fputs("be more verbose\n", stdout);
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
    const char* shortopts = "Vhvs:gi:m::r";
    bool error = false, usage = false, version = false;
#if HAVE_GETOPT_LONG
    static const struct option longopts[] = {
        { "version", no_argument,       NULL, 'V' },
        { "help",    no_argument,       NULL, 'h' },
        { "verbose", no_argument,       NULL, 'v' },
        { "serial",  required_argument, NULL, 's' },
        { "get",     no_argument,       NULL, 'g' },
        { "index",   required_argument, NULL, 'i' },
        { "mode",    optional_argument, NULL, 'm' },
        { "reset",   no_argument,       NULL, 'r' },
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
        case 'v':
            glob.verbose = true;
            break;
        case 's':
            glob.serial = optarg;
            break;
        case 'g':
            glob.get_color = true;
            break;
        case 'm':
            if (optarg) {
                char* end = NULL;
                long tmp;
                glob.set_mode = true;
                errno = 0;
                tmp = strtol(optarg, &end, 10);
                if (errno || !end || *end || tmp < 0 || tmp > 3) {
                    fprintf(stderr, "Invalid mode value: %s\n", optarg);
                    error = true;
                }
                glob.mode = tmp & 0xff;
            } else {
                glob.get_mode = true;
            }
            break;
        case 'i': {
            char* end = NULL;
            long tmp;
            errno = 0;
            tmp = strtol(optarg, &end, 10);
            if (errno || !end || *end || tmp < 0 || tmp > 255) {
                fprintf(stderr, "Invalid index value: %s\n", optarg);
                error = true;
            }
            glob.index = tmp & 0xff;
            break;
        }
        case 'r':
            glob.reset = true;
            break;
        case '?':
            error = true;
            break;
        }
    }
    if (glob.get_color) {
        if (optind >= argc) {
            glob.count = 1;
        } else if (optind + 1 == argc) {
            char* end = NULL;
            long tmp;
            errno = 0;
            tmp = strtol(argv[optind], &end, 10);
            if (errno || !end || *end || tmp <= 0 || tmp > 255) {
                fprintf(stderr, "Invalid count value: %s\n", argv[optind]);
                error = true;
            }
            glob.count = tmp & 0xff;
        } else {
            fputs("Only expects one argument when getting color\n", stderr);
            error = true;
        }
    } else {
        if (!glob.get_mode && !glob.set_mode && !glob.reset && optind == argc) {
            fputs("Expected one color after options\n", stderr);
            error = true;
        } else {
            while (optind < argc) {
                char* end = NULL;
                unsigned long tmp;
                bs_color_t* c;
                size_t offset;
                if (strlen(argv[optind]) == 7 && argv[optind][0] == '#') {
                    offset = 1;
                } else if (strlen(argv[optind]) == 8 &&
                           memcmp(argv[optind], "0x", 2) == 0) {
                    offset = 2;
                } else {
                    fprintf(stderr, "Invalid color value: %s\n", argv[optind]);
                    error = true;
                    break;
                }
                errno = 0;
                tmp = strtoul(argv[optind] + offset, &end, 16);
                if (errno || !end || *end) {
                    fprintf(stderr, "Invalid color value: %s\n", argv[optind]);
                    error = true;
                    break;
                }
                c = &glob.color[glob.index + glob.count++];
                c->red = (tmp & 0xff0000) >> 16;
                c->green = (tmp & 0xff00) >> 8;
                c->blue = (tmp & 0xff);
                optind++;
            }
        }
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

bool reset(bs_device_t* dev) {
    const unsigned int count = bs_get_max_leds(dev);
    int mode;
    if (count == 0) return false;
    mode = bs_get_mode(dev);
    if (mode == -1) return false;
    if (count == 1 || mode == BS_MODE_REPEAT) {
        bs_color_t clr = { 0 };
        return bs_set(dev, clr);
    } else {
        bs_color_t* clr = calloc(sizeof(bs_color_t), count);
        bool ret = bs_set_many(dev, count, clr);
        free(clr);
        return ret;
    }
}
