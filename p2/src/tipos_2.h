
#ifndef _tipos_h
#define _tipos_h

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_heap_task_info.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "tipos_2.h"

#define MEAN_WEIGHT_1 0.05
#define MEAN_WEIGHT_2 0.10
#define MEAN_WEIGHT_3 0.15
#define MEAN_WEIGHT_4 0.25
#define MEAN_WEIGHT_5 0.45


#define PRIORITY_SENSOR_TASK  4
#define PRIORITY_FILTER_TASK  3
#define PRIORITY_CONTROLLER_TASK  3

#define ID_SENSOR_1  1001
#define ID_SENSOR_2  1002

#define ID_TASK_1    2001
#define ID_TASK_2    2002

#define MILLIS_DELAY_SENSOR_1  5000
#define MILLIS_DELAY_SENSOR_2  7000

# define MAX_SIZE 5  

typedef struct sensor
{
    int id;
    int tag;
    int millis;
    int count;
    QueueHandle_t queue;
} sensor_t;

typedef struct sensor_data
{
    uint32_t number;
    time_t timestamp;
} sensor_data_t;

typedef struct controller
{
    int sensorId;
    int taskId;
    float data;
    time_t timestamp;
} controller_t;

#endif

