
#ifndef _custom_types_h
#define _custom_types_h

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "esp_heap_task_info.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "driver/i2c.h"

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#define I2C_MASTER_NUM 0                                    // select 0 or 1 availables
#define I2C_MASTER_SDA_IO 21                                // 21 default GPIO21
#define I2C_MASTER_SCL_IO 22                                // 22 default GPIO22 
#define I2C_MASTER_FREQ_HZ 50000                            // I2C master clock frequency. Max 1MHz
#define I2C_MASTER_RX_BUF_DISABLE 0                         // Receiving buffer size. Only slave mode will use this value, it is ignored in master mode. 
#define I2C_MASTER_TX_BUF_DISABLE 0                         // Sending buffer size. Only slave mode will use this value, it is ignored in master mode. 

#define SGP30_SENSOR_ADDR 0x58                              // SGP30 sensor slave address. 0x58 - page 7/15
#define SGP30_CMD_START 0x2003                              // Start air quality measurement- page 9/15 Table 10
#define SGP30_CMD_MEASURE 0x2008                            // Measurement- page  8/15 Table 10

#define ACK_VAL 0x1                                         // I2C ack value
#define NACK_VAL 0x0                                        // I2C nack value

#define READ_FILE "rb"
#define WRITE_FILE "wb"
#define APPEND_FILE "a"

#define PRIORITY_STATUS_HANDLER_TASK 4                      // min -->0, max --> 9

#define TAG "FINAL PROJECT"

enum machine_status{READ_CO2, SEND_CO2};
static const char *machine_status_string[] = {
    "read_CO2", "send_CO2"
};

#endif