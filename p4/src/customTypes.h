
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
#include "freertos/queue.h"
#include "driver/adc.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"
#include "driver/i2c.h"

/**
 * Config driver i2c
 */
#define I2C_MASTER_NUM 0                            // select 0 or 1 availables
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA     // default GPIO21 via menuconfig
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL     // default GPIO22 via menuconfig
#define I2C_MASTER_FREQ_HZ 400000                   // select frequency specific
#define I2C_MASTER_RX_BUF_DISABLE 0                 // Receiving buffer size. Only slave mode will use this value, it is ignored in master mode. 
#define I2C_MASTER_TX_BUF_DISABLE 0                 // Sending buffer size. Only slave mode will use this value, it is ignored in master mode. 


#define PRIORITY_STATUS_HANDLER_TASK 4      // min -->0, max --> 9

#define MILLIS_PERIODIC_SENSOR_TEMPERATURE 4000   // 10 millis

#define TAG "P4"

#endif 