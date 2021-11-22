
#ifndef _types_h
#define _types_h
/*---------------------------------------------------------------------------*/
// IMPORTS
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
//#include <float.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*---------------------------------------------------------------------------*/
// CONFIG
#define NUMBER_SENSORS 2
const int PRIORITY_SENSOR_TASK = 4;         // min -->0, max --> 9
const int PRIORITY_FILTER_TASK = 3;         // min -->0, max --> 9
const int PRIORITY_CONTROLLER_TASK = 2;     // min -->0, max --> 9
const int PRIORIDAD_TAREA_ESTADISTICAS = 1; // min -->0, max --> 9
const int WRITE_DELAY = 200;                //write delay ms if queue is full
const int READ_DELAY = 300;                 //read delay ms if queue is empty
const int NUM_ELEM_SENSOR_QUEUE = 5;
const int NUM_ELEM_FILTER_QUEUE = 5;
#define NUM_WEIGHTING_COEFFICIENTS 5
const float weightingCoefficients[NUM_WEIGHTING_COEFFICIENTS] = {0.45, 0.25, 0.15, 0.10, 0.05};
const int READ_INTERVAL = 1 * 1000;        //ms
const int CONTROLLER_INTERVAL = 1 * 1000;  //ms
const int STADISTICS_INTERVAL = 10 * 1000; //ms

const bool *initSystemDate = true; // true -> yes, false -> no)
const char *newSystemDate = "2021.02.03 20:00:00";

/*---------------------------------------------------------------------------*/
// CONSTANTS
const char *prefixReadTask = "read";             //"read_task"
const char *prefixFilterTask = "filter";         //"filter_task"
const char *prefixControllerTask = "controller"; //"controller_task"
const char *prefixSensor = "sensor";
#define dimArrayName 50
#define dimArrayNumber 20
#define NUMBER_QUEUES NUMBER_SENSORS

/*---------------------------------------------------------------------------*/
// DEFINITIONS
typedef struct
{
    long values[NUM_WEIGHTING_COEFFICIENTS];
    int index;
    int aforo;
    int aforoMax;
} CircularVector_t;

typedef struct
{
    char name[dimArrayName];
    int readingInterval;
    QueueHandle_t sensorQueue;
    QueueHandle_t filterQueue;
    char filterTaskName[dimArrayName];
    CircularVector_t filterCount;
} Sensor_t;

typedef struct
{
    int value;
    time_t timestamp;
} SensorData_t;

typedef struct
{
    char taskName[dimArrayName];
    float valueWeighted;
    int valueOriginal;
    time_t timestamp;
} FilterData_t;
#endif
