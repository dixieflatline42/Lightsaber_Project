#ifndef ADXL345_I2C_H
#define ADXL345_I2C_H

#include "main.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO    21          /*!< GPIO21 - SCL */
#define I2C_MASTER_SDA_IO    22          /*!< GPIO22 - SDA */
#define I2C_MASTER_NUM       I2C_NUM_0   /*!< I2C port number */
#define I2C_MASTER_FREQ_HZ   100000      /*!< I2C master clock frequency */
#define ADXL345_ADDR         0x53        /*!< ADXL345 device address */

void task_adxl345(void *pvParameter);
void adxl345_setup();
void adxl345_init();
void adxl345_read_data(int16_t *x, int16_t *y, int16_t *z);

#endif /* ADXL345_I2C_H */