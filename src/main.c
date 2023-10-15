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

int main(void)
{
	int err;

	if (!device_is_ready(sl_i2c1))
	{
		printk("i2c devices not ready\n");
		return 0;
	}
	printk("i2c read\n");

	uint8_t data[4];

	i2c_read(sl_i2c1, data, 4, 0x62);

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

	while (1)
	{
		led_update();
		servo_update();

		uint8_t data[4];

		i2c_read(sl_i2c1, data, 4, 0x62);
		printk("i2c %d %d %d %d\n", data[0],  data[1],  data[2],  data[3] );
		k_sleep(K_MSEC(servo_timeout));
	}
	return 0;
}
