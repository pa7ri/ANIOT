
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

#define TOUCH_PAD_NO_CHANGE (-1)
#define TOUCH_THRESH_NO_USE (0)
#define TOUCH_FILTER_MODE_EN (1)
#define TOUCH_PAD_MAX (10)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

#define SENSOR_HALL_THRESHOLD 10 // TODO: config that

#define TOUCH_PAD_PIN_0 0 // 1 second

#define MICROS_PERIODIC_CHRONOMETER 1000000 // 1 second
#define MICROS_PERIODIC_SENSOR_HALL 10000   // 10 millis
#define TAG "P3"

#endif
