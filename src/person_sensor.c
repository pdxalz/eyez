#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/i2c.h>
#include "person_sensor.h"

const struct device *const sl_i2c1 = DEVICE_DT_GET(DT_ALIAS(person_sensor));

int zero_count = 0;
person_sensor_results_t results = {};

static uint8_t num_faces = 0;

struct
{
    uint8_t left;
    uint8_t top;
    uint8_t width;
} face[PERSON_SENSOR_MAX_FACES_COUNT];

void person_sensor_init()
{
    int err;

    if (!device_is_ready(sl_i2c1))
    {
        printk("i2c devices not ready\n");
        return;
    }
    err = i2c_configure(sl_i2c1, I2C_SPEED_SET(I2C_SPEED_STANDARD));
    // err = i2c_configure(sl_i2c1, I2C_SPEED_SET(I2C_SPEED_FAST));
    if (err)
    {
        printk("i2c init failed (err %d)\n", err);
    }
    printk("i2c read\n");

    err = i2c_reg_write_byte(sl_i2c1, PERSON_ADDR, DEBUG_REG, 0);
    if (err)
    {
        printk("i2c failed (err %d)\n", err);
    }
}

static uint8_t scale100(uint8_t n)
{
    if (n < 78)
        return 0;
    if (n > 156)
        return 100;

    return (n - 78);
}

bool get_person_position(uint8_t *x, uint8_t *y)
{
    if (num_faces == 0)
        return false;

    *x = scale100(255 - face[0].left);
    *y = scale100(face[0].top);
    return true;
}


void check_person_sensor()
{
    int err;
    err = i2c_read(sl_i2c1, (uint8_t *)&results, sizeof(results), PERSON_ADDR);
    if (err)
    {
        printk("i2c failed (err %d)\n", err);
    }

    if (results.num_faces == 0)
    {
        if (++zero_count > 150)
        {
            num_faces = 0;
        }
        return;
    }

    if (num_faces > 3)
        return;

    num_faces = results.num_faces;
    zero_count = 0;

    for (uint8_t i = 0; i < num_faces; ++i)
    {
        face[i].left = results.faces[i].box_left;
        face[i].top = results.faces[i].box_top;
        face[i].width = results.faces[i].box_right - results.faces[i].box_left;
        printk("face %d: %d (%d %d) %d %d\n", i, results.faces[i].box_confidence, face[i].left, face[i].top, face[i].width, results.faces[i].box_bottom - results.faces[i].box_top);
    }
}