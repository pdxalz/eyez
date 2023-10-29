#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/i2c.h>
#include "person_sensor.h"

const struct device *const sl_i2c1 = DEVICE_DT_GET(DT_ALIAS(person_sensor));

int zz = 0;
person_sensor_results_t results = {};

static uint8_t num_faces = 0;
static uint8_t face_update = 0;
static uint8_t update_count = 0;
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

    *x = scale100(face[0].left);
    *y = scale100(face[0].top);
    return true;
}

void check_person_sensor()
{
    int err;
    if (++zz > 10)
    {
        zz = 0;
        err = i2c_read(sl_i2c1, (uint8_t *)&results, sizeof(results), 0x62);
        if (err)
        {
            printk("i2c failed (err %d)\n", err);
        }

        // don't update num_faces until we see several of the same
        if (num_faces != results.num_faces)
        {
            if (face_update == results.num_faces)
            {
                if (++update_count > 0)
                {
                    num_faces = face_update;
                    face_update = 0xff;
                    update_count = 0;
                }
                else
                {
                    return;
                }
            }
            else
            {
                face_update = results.num_faces;
                return;
            }
        }

        num_faces = results.num_faces;

        if (num_faces > 3)
            return;

        for (uint8_t i = 0; i < num_faces; ++i)
        {
            face[i].left = results.faces[i].box_left;
            face[i].top = results.faces[i].box_top;
            face[i].width = results.faces[i].box_right - results.faces[i].box_left;
            printk("face %d: %d (%d %d) %d %d\n", i, results.faces[i].box_confidence, face[i].left, face[i].top, face[i].width, results.faces[i].box_bottom - results.faces[i].box_top);
        }
    }
}