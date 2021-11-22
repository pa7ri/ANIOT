
#ifndef _custom_types_h
#define _custom_types_h

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "esp_heap_task_info.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/touch_pad.h"
#include "driver/adc.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

#define TOUCH_PAD_NO_CHANGE (-1)
#define TOUCH_THRESH_NO_USE (0)
#define TOUCH_FILTER_MODE_EN (1)
#define TOUCH_PAD_MAX (10)
#define TOUCH_THRESH_PERCENT  (80)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

#define SENSOR_HALL_THRESHOLD 0             // Default: 30, when a magnet is close <0 

#define PRIORITY_STATUS_HANDLER_TASK 4      // min -->0, max --> 9


#define TOUCH_PAD_PIN_0 0                   // PIN ID 0

#define MICROS_PERIODIC_CHRONOMETER 1000000 // 1 second
#define MICROS_PERIODIC_SENSOR_HALL 10000   // 10 millis
#define TAG "P3"

enum machine_status{COUNT_TIMER, START_STOP_TIMER, RESET_TIMER};
static const char *machine_status_string[] = {
    "count_timer", "start_stop_timer", "reset_timer",
};


typedef struct
{
    char name[dimArrayName];
    int readingInterval;
    QueueHandle_t sensorQueue;
    QueueHandle_t filterQueue;
    char filterTaskName[dimArrayName];
    CircularVector_t filterCount;
} SensorTemp_t;

#endif
