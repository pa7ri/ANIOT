
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

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#define PRIORITY_STATUS_HANDLER_TASK 4         // min -->0, max --> 9

#define MICROS_PERIODIC_SENSOR_HALL 10000000   // 10 seconds
#define MICROS_CHRONO_READER 60000000          // 1 minute
#define READ_FILE "rb"
#define WRITE_FILE "wb"
#define APPEND_FILE "a"
#define NUM_LOGS_READ 5
#define TAG "P6"

enum machine_status{WRITE_LOG, READ_LOG};
static const char *machine_status_string[] = {
    "write_log", "read_log"
};

#endif