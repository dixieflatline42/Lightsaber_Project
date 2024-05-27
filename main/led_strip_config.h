#ifndef LED_STRIP_CONFIG_H
#define LED_STRIP_CONFIG_H

#include "main.h"

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "led_strip_encoder.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      16
#define EXAMPLE_LED_NUMBERS         50
#define EXAMPLE_CHASE_SPEED_MS      100

void led_strip_setup();
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
void sabre_liga();

#endif /* LED_STRIP_CONFIG_H */