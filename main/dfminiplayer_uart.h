#ifndef DFMINIPLAYER_UART_H
#define DFMINIPLAYER_UART_H

#include "main.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

#define TXD_PIN  GPIO_NUM_16
#define RXD_PIN  GPIO_NUM_17
#define UART_NUM UART_NUM_1

void dfmplayer_uart_init();
void dfmplayer_send_cmd(uint8_t command, uint16_t parameter);
void send_raw_command(uint8_t buf[], size_t size);
void dfmplayer_reset();
void init_dfmplayer();
void set_dfmplayer_volume(int volume);
void dfmplayer_play();
void dfmplayer_next();
void dfmplayer_spec_playback(int track);
void dfmplayer_spec_playback_single_repeat(int track);
bool dfmplayer_wait_resp();

#define QUERY_STATUS 1
#define SET_VOLUME   2
#define RESET        3
#define PLAY         4
#define NOOP         0

void dfplayer_task(void *pvParameters);

#endif /* DFMINIPLAYER_UART_H */
