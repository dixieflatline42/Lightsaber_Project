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

#define BOTAO_1 4

QueueHandle_t filaDeInterrupcao;

static void IRAM_ATTR gpio_isr_handler(void *args)
{
    int pino = (int)args;
    xQueueSendFromISR(filaDeInterrupcao, &pino, NULL);

}

void trataInterrupcaoBotao(void *params)
{
    int pino;
    int contador = 0;

    while(true)
    {
        if(xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY))
        {
            //De-bouncing
            int estado = gpio_get_level(pino);
            if(estado == 1)
            {
                gpio_isr_handler_remove(pino);
                while(gpio_get_level(pino) == estado)
                {
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }

                contador++;
                // printf("O botão %d foi acionado %d vezes\n", pino, contador);
                ESP_LOGI(TAG, "O botão %d foi acionado %d vezes", pino, contador);
                
                //Habilita a interrupção novamente
                vTaskDelay(50 / portTICK_PERIOD_MS);
                gpio_isr_handler_add(pino, gpio_isr_handler, (void *) BOTAO_1);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando sistema...");

    adxl345_setup();
    led_strip_setup();

    //Configuração do pino do botão
    gpio_reset_pin(BOTAO_1);
    
    //Configura o pino do botão como entrada
    gpio_set_direction(BOTAO_1, GPIO_MODE_INPUT);
    
    //Configura o resistor de pukk down para o botão (por padrão a entrada estará em 0)
    gpio_pulldown_en(BOTAO_1);
    
    //Desabilita o resistor de pull up por segurança
    gpio_pullup_dis(BOTAO_1);
    
    //Configura pino para interrupção
    gpio_set_intr_type(BOTAO_1, GPIO_INTR_POSEDGE);

    filaDeInterrupcao = xQueueCreate(10, sizeof(int));

    if (filaDeInterrupcao == NULL) {
        printf("Erro ao criar a fila\n");
        return;
    }

    xTaskCreate(trataInterrupcaoBotao, "TrataBotao", 2048, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOTAO_1, gpio_isr_handler, (void *) BOTAO_1);

    xTaskCreate(task_adxl345, "adxl345_task", 2048, NULL, 5, NULL);
    xTaskCreate(dfplayer_task, "dfplayer_task", 4096, NULL, 5, NULL);

    while (1) {
        lightsaber_fadein();
    }
}
