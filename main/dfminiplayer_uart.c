#include "dfminiplayer_uart.h"

static const int RX_BUF_SIZE = 1024;
static const char *TAG = "DFPlayer";

void dfmplayer_uart_init() {
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_LOGI(TAG, "UART initialized.");
}

void dfmplayer_send_cmd(uint8_t command, uint16_t parameter) {
    uint8_t sendBuffer[10] = {0x7E, 0xFF, 0x06, command, 0x00, (uint8_t)(parameter >> 8), (uint8_t)(parameter & 0xFF), 0x00, 0x00, 0xEF};
    uint16_t checksum = 0xFFFF - (sendBuffer[1] + sendBuffer[2] + sendBuffer[3] + sendBuffer[4] + sendBuffer[5] + sendBuffer[6]) + 1;
    sendBuffer[7] = (uint8_t)(checksum >> 8);
    sendBuffer[8] = (uint8_t)(checksum & 0xFF);

    ESP_LOGI(TAG, "Sending command: %02X, Parameter: %04X, Checksum: %04X (%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X)",
             command, parameter, checksum,
             sendBuffer[0], sendBuffer[1], sendBuffer[2], sendBuffer[3], sendBuffer[4],
             sendBuffer[5], sendBuffer[6], sendBuffer[7], sendBuffer[8], sendBuffer[9]);
    uart_write_bytes(UART_NUM, (const char *)sendBuffer, sizeof(sendBuffer));
}

void dfmplayer_send_raw_cmd(uint8_t buf[], size_t size) {
    ESP_LOGI(TAG, "Sending raw command: (%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X)",
            buf[0], buf[1], buf[2], buf[3], buf[4],
            buf[5], buf[6], buf[7], buf[8], buf[9]);
    uart_write_bytes(UART_NUM, (const char *)buf, size);
}

void dfmplayer_reset() {
    dfmplayer_send_cmd(0x0C, 0); // Initializes the module querying its current state
}

void init_dfmplayer() {
    dfmplayer_send_cmd(0x3F, 0); // Initializes the module querying its current state
}

void set_dfmplayer_volume(int volume) {
    // 7E FF 06 06 00 00 0F FF D5 EF
    uint8_t buf[10] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x0F, 0xFF, 0xD5, 0xEF};
    dfmplayer_send_raw_cmd(buf, sizeof(buf));
}

void dfmplayer_play() {
    // 7E FF 06 06 00 00 0F FF D5 EF
    dfmplayer_send_cmd(0x0D, 0);
}

void dfmplayer_spec_playback(int track) {
    // 7E FF 06 03 00 00 01 FE F7 EF
    dfmplayer_send_cmd(0x03, track);
}

void dfmplayer_spec_playback_single_repeat(int track) {
    // 7E FF 06 03 00 00 01 FE F7 EF
    dfmplayer_send_cmd(0x08, track);
}

void dfmplayer_next() {
    // sendCommand(0x06, volume); // Set volume to 15 (max is 30)
    // 7E FF 06 06 00 00 0F FF D5 EF
    dfmplayer_send_cmd(0x01, 0);
}

bool dfmplayer_wait_resp() {
    uint8_t response[10];
    int length = uart_read_bytes(UART_NUM, response, 10, 1000 / portTICK_PERIOD_MS);
    if (length > 0) {
        ESP_LOGI(TAG, "Received response: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 response[0], response[1], response[2], response[3], response[4],
                 response[5], response[6], response[7], response[8], response[9]);
        return true;
    }
    ESP_LOGE(TAG, "No response from DFPlayer");
    return false;
}
