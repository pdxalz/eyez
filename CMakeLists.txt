# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(servo_motor)

target_sources(app PRIVATE
    src/main.c
    src/command.c
    src/led.c
    src/servo.c
    src/person_sensor.c
    src/chat_cli.c
    src/model_handler.c
)
