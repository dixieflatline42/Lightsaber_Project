#include "dfminiplayer_uart.h"

static const int RX_BUF_SIZE = 1024;
static const char *TAG = "DFPlayer";

void uart_init() {
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

void sendCommand(uint8_t command, uint16_t parameter) {
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

void send_raw_command(uint8_t buf[], size_t size) {
    ESP_LOGI(TAG, "Sending raw command: (%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X)",
            buf[0], buf[1], buf[2], buf[3], buf[4],
            buf[5], buf[6], buf[7], buf[8], buf[9]);
    uart_write_bytes(UART_NUM, (const char *)buf, size);
}

void dfmplayer_reset() {
    sendCommand(0x0C, 0); // Initializes the module querying its current state
}

void init_dfmplayer() {
    sendCommand(0x3F, 0); // Initializes the module querying its current state
}

void set_dfmplayer_volume(int volume) {
    // sendCommand(0x06, volume); // Set volume to 15 (max is 30)
    // 7E FF 06 06 00 00 0F FF D5 EF
    uint8_t buf[10] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x0F, 0xFF, 0xD5, 0xEF};
    send_raw_command(buf, sizeof(buf));
}

void dfmplayer_play() {
    // sendCommand(0x06, volume); // Set volume to 15 (max is 30)
    // 7E FF 06 06 00 00 0F FF D5 EF
    sendCommand(0x0D, 0);
}

void dfmplayer_next() {
    // sendCommand(0x06, volume); // Set volume to 15 (max is 30)
    // 7E FF 06 06 00 00 0F FF D5 EF
    sendCommand(0x01, 0);
}

bool waitForResponse() {
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

void dfplayer_task(void *pvParameters) {
    uart_init();
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for 2 seconds for DFPlayer initialization

    int state = RESET;
    while (true) {
        while (true) {
            switch (state) {
                case RESET:
                    ESP_LOGI(TAG, "Sending reset command...");
                    dfmplayer_reset();
                    break;
                case QUERY_STATUS:
                    ESP_LOGI(TAG, "Sending query status command...");
                    init_dfmplayer();
                    break;
                case SET_VOLUME:
                    ESP_LOGI(TAG, "Sending set volume command (15)...");
                    set_dfmplayer_volume(15);
                    break;
                case PLAY:
                    ESP_LOGI(TAG, "Sending play command...");
                    dfmplayer_play();
                    vTaskDelay(10000 / portTICK_PERIOD_MS);
                    dfmplayer_next();
                    break;
                default:
                    ESP_LOGI(TAG, "No more commands to send. Sleeping...");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    break;
            }
            ESP_LOGI(TAG, "Trying to read response...");
            bool received_resp = waitForResponse();
            if (received_resp) {
                switch (state) {
                    case RESET:
                        state = QUERY_STATUS;
                        break;
                    case QUERY_STATUS:
                        state = SET_VOLUME;
                        break;
                    case SET_VOLUME:
                        state = PLAY;
                        break;
                    case PLAY:
                        state = NOOP;
                        break;
                }
                break;
            } else {
                ESP_LOGI(TAG, "Failed to read response. Waiting before trying again...");
                vTaskDelay(1000 / portTICK_PERIOD_MS); // Waits before trying to read response again
            }
        }
    }

    // waitForResponse();
    // sendCommand(0x3F, 0); // Initialization command
    // vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for 2 seconds for DFPlayer initialization
    // if (!waitForResponse()) {
    //     ESP_LOGE(TAG, "Failed to initialize DFPlayer.");
    //     vTaskDelete(NULL);
    //     return;
    // }

    // vTaskDelay(500 / portTICK_PERIOD_MS); // Wait briefly between commands

    // ESP_LOGI(TAG, "Setting volume...");
    // sendCommand(0x06, 15); // Set volume to 15 (max is 30)
    // if (!waitForResponse()) {
    //     ESP_LOGE(TAG, "Failed to set volume.");
    //     vTaskDelete(NULL);
    //     return;
    // }

    // vTaskDelay(500 / portTICK_PERIOD_MS); // Wait briefly between commands

    // ESP_LOGI(TAG, "Playing first track...");
    // sendCommand(0x0F, 0x0001); // Play first track
    // if (!waitForResponse()) {
    //     ESP_LOGE(TAG, "Failed to play first track.");
    //     vTaskDelete(NULL);
    //     return;
    // }
}
