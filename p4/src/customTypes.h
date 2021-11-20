
#ifndef _custom_types_h
#define _custom_types_h

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "esp_heap_task_info.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"
#include "driver/i2c.h"

/**
 * Config driver i2c
 */
#define I2C_MASTER_NUM 0                                    // select 0 or 1 availables
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA             // 21 default GPIO21 via menuconfig
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL             // 22 default GPIO22 via menuconfig
#define I2C_MASTER_FREQ_HZ 50000                            // I2C master clock frequency. Max 1MHz
#define I2C_MASTER_RX_BUF_DISABLE 0                         // Receiving buffer size. Only slave mode will use this value, it is ignored in master mode. 
#define I2C_MASTER_TX_BUF_DISABLE 0                         // Sending buffer size. Only slave mode will use this value, it is ignored in master mode. 

#define SI7021_SENSOR_ADDR CONFIG_SI7021_I2C_SLAVE_ADDRESS  // Si7021 sensor slave address. 1000000 : 0x40 - page 18
#define SI7021_CMD_START 0xF3                               // Start measure temp for Si7021 sensor. Measure Temperature, No Hold Master Mode 0xF3 - page 18

#define ACK_VAL 0x1                                         // I2C ack value
#define NACK_VAL 0x0                                        // I2C nack value

#define PRIORITY_TASK 4                                     // min -->0, max --> 9

#define TAG "P4"

#endif 