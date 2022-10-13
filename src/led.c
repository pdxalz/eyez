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

static const struct led_rgb colors[] = {
    RGB(0x0f, 0x00, 0x00), /* red */
    RGB(0x00, 0x0f, 0x00), /* green */
    RGB(0x00, 0x00, 0x0f), /* blue */
};

struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);
static size_t cursor = 0, color = 0;

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

void led_update()
{
    int rc;

    memset(&pixels, 0x00, sizeof(pixels));
 //   memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));
    memcpy(&pixels[0], &colors[color], sizeof(struct led_rgb));
    memcpy(&pixels[1], &colors[(color+1)%3], sizeof(struct led_rgb));
    rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

    if (rc)
    {
        printk("couldn't update strip: %d", rc);
    }

    cursor++;
    if (cursor >= STRIP_NUM_PIXELS)
    {
        cursor = 0;
        color++;
        if (color == ARRAY_SIZE(colors))
        {
            color = 0;
        }
    }
}