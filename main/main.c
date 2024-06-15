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

#define ON_OFF_BUTTON_PIN       4
#define CHANGE_COLOR_BUTTON_PIN 13

QueueHandle_t on_off_queue;
QueueHandle_t change_color_queue;
QueueHandle_t sound_queue;

typedef enum {OFF = 0, ON} state_t;

typedef struct color_t { int r, g, b; } color_t;

#define RED   (color_t){.r = 255, .g = 0,   .b = 0   }
#define GREEN (color_t){.r = 0,   .g = 255, .b = 0   }
#define BLUE  (color_t){.r = 0,   .g = 0,   .b = 255 }
color_t colors[] = {RED, GREEN, BLUE};

typedef enum {
    SOUND_ON = 1,
    SOUND_STALE = 2,
    SOUND_MOVEMENT = 3,
    SOUND_OFF = 4,
} sound_cmd_t;

// TODO: this struct should be atomic, to avoid concurrent state changes.
typedef struct lightsaber_state_t {
    state_t state;
    color_t color;
} lightsaber_state_t;

lightsaber_state_t lightsaber_state = { .state = OFF, .color = BLUE };

// Global interruption handler.
static void IRAM_ATTR gpio_isr_handler(void *args) {
    int pin = (int)args;
    switch (pin) {
        case ON_OFF_BUTTON_PIN:
            xQueueSendFromISR(on_off_queue, &pin, NULL);
            break;
        case CHANGE_COLOR_BUTTON_PIN:
            xQueueSendFromISR(change_color_queue, &pin, NULL);
            break;
    }
}

void on_off_queue_handler(void *params) {
    while (true) {
        int pin;
        if(xQueueReceive(on_off_queue, &pin, portMAX_DELAY)) {
            gpio_isr_handler_remove(pin);

            if (lightsaber_state.state == OFF) {
                ESP_LOGI(TAG, "Turning lightsaber ON");
                sound_cmd_t sound_cmd = SOUND_ON;
                xQueueSendFromISR(sound_queue, &sound_cmd, NULL);
                color_t color = lightsaber_state.color;
                lightsaber_fadein(color.r, color.g, color.b);
                lightsaber_state.state = ON;
            } else {
                lightsaber_state.state = OFF;
                ESP_LOGI(TAG, "Turning lightsaber OFF");
                sound_cmd_t sound_cmd = SOUND_OFF;
                xQueueSendFromISR(sound_queue, &sound_cmd, NULL);
                color_t color = lightsaber_state.color;
                lightsaber_fadeout(color.r, color.g, color.b);
            }

            gpio_isr_handler_add(pin, gpio_isr_handler, (void*)pin);
        }
    }
}

void change_color_queue_handler(void *params) {
    int color_idx = 0;
    while (true) {
        int pin;
        if(xQueueReceive(change_color_queue, &pin, portMAX_DELAY)) {
            if (lightsaber_state.state == ON) {
                gpio_isr_handler_remove(pin);
                color_idx = (color_idx + 1) % 3;
                color_t color = colors[color_idx];
                lightsaber_state.color = color;
                ESP_LOGI(TAG, "Changing saber color to (%d, %d, %d) %d",
                         color.r, color.g, color.b, color_idx);
                lightsaber_turnon(color.r, color.g, color.b);
                gpio_isr_handler_add(pin, gpio_isr_handler, (void*)pin);
            } else {
                ESP_LOGI(TAG, "Can't change lightsaber color when it's off");
            }
        }
    }
}


void sound_queue_handler(void* params) {
    while (true) {
        sound_cmd_t cmd;
        if(xQueueReceive(sound_queue, &cmd, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Received sound command: %d", cmd);
            switch (cmd) {
                case SOUND_ON:
                    dfmplayer_spec_playback(cmd);
                    dfmplayer_wait_resp();
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    sound_cmd_t sound_cmd = SOUND_STALE;
                    xQueueSendFromISR(sound_queue, &sound_cmd, NULL);
                    break;
                case SOUND_STALE:
                    dfmplayer_spec_playback_single_repeat(cmd);
                    dfmplayer_wait_resp();
                    break;
                case SOUND_OFF:
                    dfmplayer_spec_playback(cmd);
                    dfmplayer_wait_resp();
                    break;
                default:
                    break;
            }
        }
    }
}

// Configures a pin, setting it as an input pin, configuring it with a pull-down
// resistor and attaching it to an interruption.
void config_isr_pin(int pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_pulldown_en(pin);
    gpio_pullup_dis(pin);
    gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing the saber system...");

    adxl345_setup();
    led_strip_setup();
    dfmplayer_uart_init();

    config_isr_pin(ON_OFF_BUTTON_PIN);
    config_isr_pin(CHANGE_COLOR_BUTTON_PIN);

    on_off_queue       = xQueueCreate(10, sizeof(int));
    change_color_queue = xQueueCreate(10, sizeof(int));
    sound_queue        = xQueueCreate(10, sizeof(int));

    xTaskCreate(on_off_queue_handler, "on_off_queue_handler", 2048, NULL, 1, NULL);
    xTaskCreate(change_color_queue_handler, "change_color_queue_handler", 2048, NULL, 1, NULL);
    xTaskCreate(sound_queue_handler, "sound_queue_handler", 2048, NULL, 1, NULL);

    lightsaber_turnon(0, 0, 0); // actually turns it off
    dfmplayer_reset();
    dfmplayer_wait_resp();

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ON_OFF_BUTTON_PIN,       gpio_isr_handler, (void*)ON_OFF_BUTTON_PIN);
    gpio_isr_handler_add(CHANGE_COLOR_BUTTON_PIN, gpio_isr_handler, (void*)CHANGE_COLOR_BUTTON_PIN);

    /* xTaskCreate(task_adxl345, "adxl345_task", 2048, NULL, 5, NULL); */
    /* xTaskCreate(dfplayer_task, "dfplayer_task", 4096, NULL, 5, NULL); */
}
