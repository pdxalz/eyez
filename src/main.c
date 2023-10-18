/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM-based servomotor control
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>

#include "command.h"
#include "led.h"
#include "servo.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/dk_prov.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(chat, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/i2c.h>

const struct device *const sl_i2c1 = DEVICE_DT_GET(DT_ALIAS(person_sensor));

static void bt_ready(int err)
{
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	dk_leds_init();
	dk_buttons_init(NULL);

	err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init());
	if (err)
	{
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		settings_load();
	}

	/* This will be a no-op if settings_load() loaded provisioning info */
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	printk("Mesh initialized\n");
}

extern int32_t servo_timeout;



// The person sensor will never output more than four faces.
#define PERSON_SENSOR_MAX_FACES_COUNT (4)

// How many different faces the sensor can recognize.
#define PERSON_SENSOR_MAX_IDS_COUNT (7)


// The following structures represent the data format returned from the person
// sensor over the I2C communication protocol. The C standard doesn't
// guarantee the byte-wise layout of a regular struct across different
// platforms, so we add the non-standard (but widely supported) __packed__
// attribute to ensure the layouts are the same as the wire representation.

// The results returned from the sensor have a short header providing
// information about the length of the data packet:
//   reserved: Currently unused bytes.
//   data_size: Length of the entire packet, excluding the header and checksum.
//     For version 1.0 of the sensor, this should be 40.
typedef struct __attribute__ ((__packed__)) {
    uint8_t reserved[2];  // Bytes 0-1.
    uint16_t data_size;   // Bytes 2-3.
} person_sensor_results_header_t;

// Each face found has a set of information associated with it:
//   box_confidence: How certain we are we have found a face, from 0 to 255.
//   box_left: X coordinate of the left side of the box, from 0 to 255.
//   box_top: Y coordinate of the top edge of the box, from 0 to 255.
//   box_width: Width of the box, where 255 is the full view port size.
//   box_height: Height of the box, where 255 is the full view port size.
//   id_confidence: How sure the sensor is about the recognition result.
//   id: Numerical ID assigned to this face.
//   is_looking_at: Whether the person is facing the camera, 0 or 1.
typedef struct __attribute__ ((__packed__)) {
    uint8_t box_confidence;   // Byte 1.
    uint8_t box_left;         // Byte 2.
    uint8_t box_top;          // Byte 3.
    uint8_t box_right;        // Byte 4.
    uint8_t box_bottom;       // Byte 5.
    int8_t id_confidence;     // Byte 6.
    int8_t id;                // Byte 7
    uint8_t is_facing;        // Byte 8.
} person_sensor_face_t;

// This is the full structure of the packet returned over the wire from the
// sensor when we do an I2C read from the peripheral address.
// The checksum should be the CRC16 of bytes 0 to 38. You shouldn't need to
// verify this in practice, but we found it useful during our own debugging.
typedef struct __attribute__ ((__packed__)) {
    person_sensor_results_header_t header;                     // Bytes 0-4.
    int8_t num_faces;                                          // Byte 5.
    person_sensor_face_t faces[PERSON_SENSOR_MAX_FACES_COUNT]; // Bytes 6-37.
    uint16_t checksum;                                         // Bytes 38-39.
} person_sensor_results_t;


int main(void)
{
	int err;

	if (!device_is_ready(sl_i2c1))
	{
		printk("i2c devices not ready\n");
		return 0;
	}
	err = i2c_configure(sl_i2c1, I2C_SPEED_SET(I2C_SPEED_STANDARD));
	// err = i2c_configure(sl_i2c1, I2C_SPEED_SET(I2C_SPEED_FAST));
	if (err)
	{
		printk("i2c init failed (err %d)\n", err);
	}
	printk("i2c read\n");

	command_init();
	led_init();
	servo_init();
	eye_position(Eye_both, 5);
	//	cmd_print(">>> Shell Initialized <<<");

	err = bt_enable(bt_ready);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
	}

	int zz = 0;
	person_sensor_results_t results = {};

	while (1)
	{
		led_update();
		servo_update();

		if (++zz > 10)
		{
			zz = 0;
			err = i2c_read(sl_i2c1, (uint8_t *)&results, sizeof(results), 0x62);
			if (err)
			{
				printk("i2c failed (err %d)\n", err);
			}
			printk("i2c %d %d %d %d %d\n", results.num_faces, results.faces[0].box_top, results.faces[0].box_bottom, results.faces[0].box_left, results.faces[0].box_right);
		}
		k_sleep(K_MSEC(servo_timeout));
	}
	return 0;
}
