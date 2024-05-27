/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "main.h"
// static const char *TAG = "Main";


void app_main(void)
{
    led_strip_setup();

    while (1) {
        sabre_liga();
    }
}
