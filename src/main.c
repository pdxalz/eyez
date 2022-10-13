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

#include <zephyr/zephyr.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>

#include "command.h"
#include "led.h"
#include "servo.h"

extern int32_t servo_timeout;

void main(void)
{
	command_init();
	led_init();
	servo_init();
	eye_position(Eye_both, 5);
	cmd_print(">>> Shell Initialized <<<");

	while (1)
	{
		led_update();
		servo_update();

		k_sleep(K_MSEC(servo_timeout));
	}
}
