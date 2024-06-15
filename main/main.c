/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "main.h"
static const char *TAG = "Main";

/*
Features:
    Led Strip
    Button
    Df Mini Player
    Acellerometer
*/

#define ON_OFF_BUTTON_PIN 4

QueueHandle_t on_off_queue;

typedef enum {OFF = 0, ON} state_t;

// TODO: this struct should be atomic, to avoid concurrent state changes.
typedef struct {
    state_t state;
} lightsaber_state_t;

lightsaber_state_t lightsaber_state = { .state = OFF };

// Global interruption handler.
static void IRAM_ATTR gpio_isr_handler(void *args) {
    int pino = (int)args;
    xQueueSendFromISR(on_off_queue, &pino, NULL);
}

void on_off_queue_handler(void *params) {
    while (true) {
        int pin;
        if(xQueueReceive(on_off_queue, &pin, portMAX_DELAY)) {
            gpio_isr_handler_remove(pin);

            if (lightsaber_state.state == OFF) {
                lightsaber_state.state = ON;
                ESP_LOGI(TAG, "Turning lightsaber ON");
                lightsaber_fadein();
            } else {
                lightsaber_state.state = OFF;
                ESP_LOGI(TAG, "Turning lightsaber OFF");
                lightsaber_fadeout();
            }

            gpio_isr_handler_add(pin, gpio_isr_handler, (void*)ON_OFF_BUTTON_PIN);
        }
    }
}

// Configures the ON/OFF button, setting it as an input pin, configuring it with
// a pull-down resistor and attaching it to an interruption.
void config_on_off_button(int pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_pulldown_en(pin);
    gpio_pullup_dis(pin);
    gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando sistema...");

    adxl345_setup();
    led_strip_setup();

    config_on_off_button(ON_OFF_BUTTON_PIN);

    on_off_queue = xQueueCreate(10, sizeof(int));
    xTaskCreate(on_off_queue_handler, "on_off_queue_handler", 2048, NULL, 1, NULL);

    lightsaber_turnoff();

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ON_OFF_BUTTON_PIN, gpio_isr_handler, (void*)ON_OFF_BUTTON_PIN);

    /* xTaskCreate(task_adxl345, "adxl345_task", 2048, NULL, 5, NULL); */
    /* xTaskCreate(dfplayer_task, "dfplayer_task", 4096, NULL, 5, NULL); */
}
