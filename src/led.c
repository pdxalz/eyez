#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>

#include "led.h"

#define STRIP_NODE DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)

#define RGB(_r, _g, _b)                 \
    {                                   \
        .r = (_r), .g = (_g), .b = (_b) \
    }

struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);
static long count = 0;

void led_init()
{
    if (device_is_ready(strip))
    {
        printk("Found LED strip device %s", strip->name);
    }
    else
    {
        printk("LED strip device %s is not ready", strip->name);
        return;
    }
    printk("Displaying pattern on strip");
}


struct led_rgb color_table[] = {
    RGB(0x08, 0x0f, 0x0d), // a quamarine
    RGB(0x00, 0x00, 0x0f), // b
    RGB(0x00, 0x0f, 0x0f), // c
    RGB(0x0c, 0x0a, 0x07), // d esert
    RGB(0x0c, 0x00, 0x0f), // e lectric purple
    RGB(0x05, 0x08, 0x04), // f ern
    RGB(0x00, 0x0f, 0x00), // g
    RGB(0x0f, 0x07, 0x0b), // h ot pink
    RGB(0x05, 0x00, 0x08), // i ndego
    RGB(0x00, 0x0b, 0x07), // j ade
    RGB(0x0c, 0x0b, 0x09), // k haki
    RGB(0x0c, 0x0f, 0x00), // l ime
    RGB(0x0f, 0x00, 0x0f), // m
    RGB(0x04, 0x0f, 0x01), // n eon green
    RGB(0x0f, 0x0a, 0x00), // o range
    RGB(0x02, 0x0b, 0x0d), // p acific blue
    RGB(0x0b, 0x0b, 0x0b), // q uick silver
    RGB(0x0f, 0x00, 0x00), // r
    RGB(0x0f, 0x03, 0x00), // s carlet
    RGB(0x0d, 0x0b, 0x09), // t an
    RGB(0x04, 0x00, 0x0f), // u ltramarine
    RGB(0x0e, 0x04, 0x03), // v ermillion
    RGB(0x0f, 0x0f, 0x0f), // w
    RGB(0x00, 0x00, 0x00), // x
    RGB(0x0f, 0x0f, 0x00), // y
    RGB(0x0, 0x02, 0x0b),  // z affre
};


char * color_patterns[] = {
    "ggyyyyyyyy",
    "oobbgggggg",
    "ccrroooooo",
    "xxxxxxxxxx",
    "zzzzzzzzzz",
    "lluupppppp",
    "hhjjjjjjjj",
    "vvyyoooooo",
    "rrmmmmmmmm",
    "bbcccccccc",
    "yyrroooooo",
    "pppppppppp",
    "aapppppppp",
    "ssxxxxxxxx"
};

uint8_t color_index(char color)
{
    if (color < 'a' || color > 'z')
    {
        return 0;
    }
    return color - 'a';
}

void set_color_string(const uint8_t *str)
{
    int rc;
    int length = MIN(sizeof(pixels), strlen(str));

    memset(&pixels, 0x00, sizeof(pixels));

    for (int i = 0; i < length; ++i)
    {
        uint8_t index = color_index(str[i]);
        memcpy(&pixels[i], &color_table[index], sizeof(struct led_rgb));
    }
    rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

    if (rc)
    {
        printk("couldn't update strip: %d", rc);
    }
}

void led_update()
{
    memset(&pixels, 0x00, sizeof(pixels));

    ++count;
    if (count % 100 == 1)
        set_color_string("rrggbbccmm");
    else if (count % 100 == 50)
        set_color_string("ccmmyyrrggbb");

}

static int current_pattern = 0;
void set_color_pattern(int index)
{
    current_pattern = (index % ARRAY_SIZE(color_patterns) );
    set_color_string(color_patterns[current_pattern]);
}

void next_color_pattern()
{
    set_color_pattern(current_pattern + 1);
}