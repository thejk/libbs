#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "libbs.h"
#include "extra_compiler_stuff.h"

#if HAVE_GETOPT_LONG
# include <getopt.h>
#endif
#if HAVE_PULSEAUDIO
# include <pulse/pulseaudio.h>
#endif

static struct {
    const char* serial;
    uint16_t leds;
    bool quit;

#if HAVE_PULSEAUDIO
    const char* pulse_match_source;
#endif
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
#if HAVE_PULSEAUDIO
#if HAVE_GETOPT_LONG
    fputs("  -o, --output=OUTPUT    ", stdout);
#else
    fputs("  -o OUTPUT              ", stdout);
#endif
#endif
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
    const char* shortopts = "Vhs:"
#if HAVE_PULSEAUDIO
        "o:"
#endif
        ;
    bool error = false, usage = false, version = false;
#if HAVE_GETOPT_LONG
    static const struct option longopts[] = {
        { "version", no_argument,       NULL, 'V' },
        { "help",    no_argument,       NULL, 'h' },
        { "serial",  required_argument, NULL, 's' },
#if HAVE_PULSEAUDIO
        { "output",  required_argument, NULL, 'o' },
#endif
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
#if HAVE_PULSEAUDIO
        case 'o':
            glob.pulse_match_source = optarg;
            break;
#endif
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

static void scale(bs_color_t* clr, double value) {
    if (value >= 1.0) return;
    if (value <= 0.0) {
        memset(clr, 0, sizeof(bs_color_t));
        return;
    }
    if (clr->red) {
        clr->red = round(clr->red * value);
    }
    if (clr->green) {
        clr->green = round(clr->green * value);
    }
    if (clr->blue) {
        clr->blue = round(clr->blue * value);
    }
}

static void merge(bs_color_t* clr, bs_color_t left, double value,
                  bs_color_t right) {
    if (value <= 0.0) {
        *clr = left;
    } else if (value >= 1.0) {
        *clr = right;
    } else {
        const double inv_value = 1.0 - value;
        clr->red = round(left.red * value + right.red * inv_value);
        clr->green = round(left.green * value + right.green * inv_value);
        clr->blue = round(left.blue * value + right.blue * inv_value);
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
        scale(table + (num - 1), 1.0 - num + blue_part);
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

static bool set_value(bs_device_t* dev, double value, bs_color_t* table,
                      const bs_color_t* blue_table,
                      const bs_color_t* normal_table) {
    if (glob.leds == 1) {
        if (value <= 0.0) {
            *table = blue;
        } else {
            *table = green;
            scale(table, value);
        }
        return bs_set(dev, *table);
    }
    if (value <= 0.0) {
        memcpy(table, blue_table, sizeof(bs_color_t) * glob.leds);
    } else {
        const double fill = glob.leds * value;
        const size_t high = ceil(fill);
        memcpy(table, normal_table, sizeof(bs_color_t) * high);
        memset(table + high, 0, sizeof(bs_color_t) * (glob.leds - high));
        if (high > 0) {
            scale(table + high - 1, 1.0 - high + fill);
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

#if HAVE_PULSEAUDIO
typedef struct pulse_data_t {
    pa_mainloop* loop;
    pa_mainloop_api* loop_api;
    pa_stream* stream;
    bs_device_t* dev;
    bs_color_t* table;
    const bs_color_t* blue_table;
    const bs_color_t* normal_table;
} pulse_data_t;

static void stream_suspended_cb(pa_stream* stream, void* userdata) {
    if (pa_stream_is_suspended(stream)) {
        pulse_data_t* data = userdata;
        set_value(data->dev, 0, data->table, data->blue_table,
                  data->normal_table);
    }
}

static void stream_read_cb(pa_stream* stream, size_t length, void* userdata) {
    pulse_data_t* data = userdata;
    const void *ptr;
    double value;

    if (pa_stream_peek(stream, &ptr, &length) < 0) {
        return;
    }

    assert(length > 0);
    assert(length % sizeof(float) == 0);

    value = ((const float*)ptr)[length / sizeof(float) - 1];

    pa_stream_drop(stream);

    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;

    set_value(data->dev, value,
              data->table, data->blue_table, data->normal_table);
}

static void source_info_cb(pa_context* ctx, const pa_source_info* info, int eol,
                           void* userdata) {
    pulse_data_t* data = userdata;
    pa_sample_spec samplespec;
    pa_buffer_attr attr;
    if (!info) {
        if (eol && !data->stream) {
            // No sink found
            data->loop_api->quit(data->loop_api, 0);
        }
        return;
    }
    if (data->stream) return;
    samplespec.format = PA_SAMPLE_FLOAT32NE;
    samplespec.channels = 1;
    samplespec.rate = 25;
    memset(&attr, 0, sizeof(attr));
    attr.fragsize = sizeof(float);
    attr.maxlength = (uint32_t)-1;
    data->stream = pa_stream_new(ctx, "Peak detect", &samplespec, NULL);
    if (!data->stream) return;
    pa_stream_set_read_callback(data->stream, stream_read_cb, data);
    pa_stream_set_suspended_callback(data->stream, stream_suspended_cb, data);
    if (pa_stream_connect_record(data->stream, info->name, &attr,
                                 PA_STREAM_DONT_INHIBIT_AUTO_SUSPEND |
                                 PA_STREAM_PEAK_DETECT |
                                 PA_STREAM_ADJUST_LATENCY) < 0) {
        fputs("Error connecting to peak detector\n", stderr);
        pa_stream_unref(data->stream);
        data->stream = NULL;
    }
}

static void context_state_cb(pa_context* ctx, void* userdata) {
    switch (pa_context_get_state(ctx)) {
    case PA_CONTEXT_READY: {
        pa_operation* oper;
        if (glob.pulse_match_source) {
            oper = pa_context_get_source_info_by_name(ctx,
                                                      glob.pulse_match_source,
                                                      source_info_cb,
                                                      userdata);
        } else {
            oper = pa_context_get_source_info_list(ctx, source_info_cb,
                                                   userdata);
        }
        pa_operation_unref(oper);
        break;
    }
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED: {
        pulse_data_t* data = userdata;
        data->loop_api->quit(data->loop_api, 0);
        break;
    }
    default:
        break;
    }
}

#endif  // HAVE_PULSEAUDIO

bool run_capture(bs_device_t* dev, bs_color_t* table,
                 const bs_color_t* blue_table,
                 const bs_color_t* normal_table) {
#if HAVE_PULSEAUDIO
    pulse_data_t data;
    pa_proplist* proplist;
    pa_context* ctx;
    bool ret;

    memset(&data, 0, sizeof(data));
    data.dev = dev;
    data.table = table;
    data.blue_table = blue_table;
    data.normal_table = normal_table;

    data.loop = pa_mainloop_new();
    data.loop_api = pa_mainloop_get_api(data.loop);
    proplist = pa_proplist_new();
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME,
                     "Volume Meter for BlinkStick");
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_VERSION, VERSION);
    ctx = pa_context_new_with_proplist(data.loop_api, NULL, proplist);
    pa_proplist_free(proplist);

    pa_context_set_state_callback(ctx, context_state_cb, &data);
    if (pa_context_connect(ctx, NULL, PA_CONTEXT_NOFAIL, NULL) < 0) {
        pa_context_unref(ctx);
        pa_mainloop_free(data.loop);
        return false;
    }

    ret = true;
    while (!glob.quit) {
        if (pa_mainloop_iterate(data.loop, 1, NULL) < 0) {
            ret = false;
            break;
        }
    }
    if (data.stream) {
        pa_stream_unref(data.stream);
    } else {
        if (glob.pulse_match_source) {
            fprintf(stderr, "No output matching '%s' found\n",
                    glob.pulse_match_source);
        } else {
            fputs("No output found\n", stderr);
        }
    }
    pa_context_unref(ctx);
    pa_mainloop_free(data.loop);
    return ret;
#else
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
        if (!set_value(dev, value / 255.0, table, blue_table, normal_table)) {
            return false;
        }
        sleep(1);
    }
    return true;
#endif  /* FALLBACK */
}
