/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "main.h"
// static const char *TAG = "Main";

void app_main(void)
{
    adxl345_setup();
    led_strip_setup();

    xTaskCreate(task_adxl345, "adxl345_task", 2048, NULL, 5, NULL);
    
    while (1) {
        lightsaber_fadein();
    }
}
