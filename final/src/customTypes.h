
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

#define TAG "FINAL PROJECT"

enum machine_status{READ_CO2, READ_INFRARRED};
static const char *machine_status_string[] = {
    "read_CO2", "read_infrarred"
};

#endif