#include "led_strip_config.h"

static const char *TAG = "Led_strip_config";

rmt_channel_handle_t setup_led_strip_channel(void);
rmt_encoder_handle_t setup_led_strip_encoder(void);
uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

uint32_t red = 0;
uint32_t green = 50; // Definindo verde com intensidade máxima (255)
uint32_t blue = 0;

rmt_channel_handle_t led_chan;
rmt_tx_channel_config_t tx_chan_config;
rmt_encoder_handle_t led_encoder;
led_strip_encoder_config_t encoder_config;
rmt_transmit_config_t tx_config; 

void led_strip_setup()
{
    ESP_LOGI(TAG, "Create RMT TX channel");
    led_chan = NULL;
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.gpio_num = RMT_LED_STRIP_GPIO_NUM;
    tx_chan_config.mem_block_symbols = 64;
    tx_chan_config.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ;
    tx_chan_config.trans_queue_depth = 4;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_encoder = NULL;
    encoder_config.resolution = RMT_LED_STRIP_RESOLUTION_HZ;
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    ESP_LOGI(TAG, "Start LED sequence");

    tx_config.loop_count = 0;
}

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void lightsaber_fadein(int r, int g, int b)
{
    // Ligar os LEDs sequencialmente na cor verde
    for (int j = 0; j < EXAMPLE_LED_NUMBERS/2; j++) {
        led_strip_pixels[j * 3 + 0] = g; // Componente R: 0
        led_strip_pixels[j * 3 + 1] = r;     // Componente G: 255 (verde)
        led_strip_pixels[j * 3 + 2] = b;     // Componente B: 0

        // Acende LEDs da segunda metade (244 a 145, na ordem inversa)
        int reverse_index = EXAMPLE_LED_NUMBERS - j;
        led_strip_pixels[reverse_index * 3 + 0] = g;
        led_strip_pixels[reverse_index * 3 + 1] = r;
        led_strip_pixels[reverse_index * 3 + 2] = b;

        ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
    }
}

void lightsaber_fadeout(int r, int g, int b)
{
    // Ligar os LEDs sequencialmente na cor verde
    for (int j = (EXAMPLE_LED_NUMBERS/2)-1; j >= 0; j--) {
        led_strip_pixels[j * 3 + 0] = 0; // Componente R: 0
        led_strip_pixels[j * 3 + 1] = 0; // Componente G: 255 (verde)
        led_strip_pixels[j * 3 + 2] = 0; // Componente B: 0

        // Acende LEDs da segunda metade (244 a 145, na ordem inversa)
        int reverse_index = EXAMPLE_LED_NUMBERS - j;
        led_strip_pixels[reverse_index * 3 + 0] = 0;
        led_strip_pixels[reverse_index * 3 + 1] = 0;
        led_strip_pixels[reverse_index * 3 + 2] = 0;

        ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
    }
}

void lightsaber_turnon(int r, int g, int b) {
    // Ligar os LEDs sequencialmente na cor verde
    for (int j = (EXAMPLE_LED_NUMBERS/2)-1; j >= 0; j--) {
        led_strip_pixels[j * 3 + 0] = g; // Componente R: 0
        led_strip_pixels[j * 3 + 1] = r; // Componente G: 255 (verde)
        led_strip_pixels[j * 3 + 2] = b; // Componente B: 0

        // Acende LEDs da segunda metade (244 a 145, na ordem inversa)
        int reverse_index = EXAMPLE_LED_NUMBERS - j;
        led_strip_pixels[reverse_index * 3 + 0] = g;
        led_strip_pixels[reverse_index * 3 + 1] = r;
        led_strip_pixels[reverse_index * 3 + 2] = b;
    }
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}
