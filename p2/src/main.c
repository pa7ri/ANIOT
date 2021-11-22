/* Santiago Zomeño Alcala
   Patricia Barrios Palacios
   -------------------------
   miot.aniot.p2-gestion de tareas en esp32 (FreeRTOS)
   Oct/2022
*/
/*---------------------------------------------------------------------------*/
#include "types.h"

static Sensor_t sensors[NUMBER_SENSORS];
static QueueHandle_t sensorQueues[NUMBER_QUEUES];
static QueueHandle_t filterQueues[NUMBER_QUEUES];
static QueueSetHandle_t controllerQueues;
char numeroEnString[20];
char taskName[dimArrayName];
/*---------------------------------------------------------------------------*/
// INICIALIZACIONES
static void init()
{ //Init sensors
    char sensorName[dimArrayName];
    char numeroSensor[dimArrayNumber];
    for (int indexSensor = 0; indexSensor < NUMBER_SENSORS; indexSensor++)
    { //name
        strcpy(sensorName, "");
        strcat(sensorName, prefixSensor);
        strcat(sensorName, "_");
        snprintf(numeroSensor, 21, "%d", indexSensor);
        strcat(sensorName, numeroSensor);
        strcpy(sensors[indexSensor].name, sensorName);
        //filterCount
        sensors[indexSensor].filterCount.aforo = 0;
        sensors[indexSensor].filterCount.aforoMax = NUM_WEIGHTING_COEFFICIENTS;
        sensors[indexSensor].filterCount.index = sensors[indexSensor].filterCount.aforoMax - 1;
        for (int indAcumulador = 0; indAcumulador < sensors[indexSensor].filterCount.aforoMax; indAcumulador++)
        {
            sensors[indexSensor].filterCount.values[indAcumulador] = 0;
        }
    }

    //Init systemtime
    if (!initSystemDate)
    {
        time_t newDate = 0;
        int year = 0, month = 0, day = 0, hours = 0, minutes = 0, seconds = 0;
        sscanf(newSystemDate, "%4d.%2d.%2d %2d:%2d:%2d", &year, &month, &day, &hours, &minutes, &seconds);
        struct tm infoTime = {0};
        infoTime.tm_year = year - 1900; /* years since 1900 */
        infoTime.tm_mon = month - 1;
        infoTime.tm_mday = day;
        infoTime.tm_hour = hours;
        infoTime.tm_min = minutes;
        infoTime.tm_sec = seconds;
        newDate = mktime(&infoTime);
        struct timeval newDateFormat = {.tv_sec = newDate};
        settimeofday(&newDateFormat, NULL);
    }
}
/*---------------------------------------------------------------------------*/
// UTILS
static int readSensor(char *sensorSolicitado)
{
    int value = esp_random() % 100;
    return value;
}
static void insertValue(int32_t value, CircularVector_t *circularVector, char *sensorName)
{
    if (circularVector->index == 0)
    {
        circularVector->index = circularVector->aforoMax;
    }
    circularVector->index--;

    //int outdatedValue = circularVector->values[circularVector->index];
    //printf("%s.Weighted counter. Getting:%d\n",sensorName,outdatedValue);

    circularVector->values[circularVector->index] = value;
    //printf("%s.Weighted counter. Inserting:%d\n",sensorName,valor);
}
static float getWeightedMean(CircularVector_t circularVector, const float coefficients[], char *sensorName)
{ //printf("%s.Weighted mean. Elem: [",sensorName);

    float weightedMean = 0;
    int indCoeficientes = 0;
    for (int i = circularVector.index; i < circularVector.aforoMax; i++)
    {
        weightedMean += circularVector.values[i] * coefficients[indCoeficientes];
        //printf("(%li * %g) + ",circularVector.values[i], coefficients[indCoeficientes]);
        indCoeficientes++;
    }
    for (int i = 0; i < circularVector.index; i++)
    {
        weightedMean += circularVector.values[i] * coefficients[indCoeficientes];
        //printf("(%li * %lg) + ",circularVector.values[i], coefficients[indCoeficientes]);
        indCoeficientes++;
    }
    //printf("]: %g\n",weightedMean);

    return weightedMean;
}
static void showStadistics()
{
    char stats[4096];
    vTaskGetRunTimeStats(stats);
    printf("\nTask stadistics in cpu:\n%s\n",stats);
}
/*---------------------------------------------------------------------------*/
// TASKS

static void sensorTask(void *args)
{
    Sensor_t *sensor = (Sensor_t *)args;
    uint32_t sensorValue = 0;
    time_t currentTimestamp;
    SensorData_t sensorData;
    QueueHandle_t sensorQueue = sensor->sensorQueue;
    BaseType_t statusSensorQueue;

    //printf("\n-----------------------------------------\n");
    while (1)
    {
        sensorValue = readSensor(sensor->name);
        currentTimestamp = time(NULL);
        sensorData.value = sensorValue;
        sensorData.timestamp = currentTimestamp;
        printf("%s.Generated data:%d (each %d ms) %s", sensor->name, sensorValue, sensor->readingInterval, ctime(&currentTimestamp));

        statusSensorQueue = xQueueSendToBack(sensorQueue, &sensorData, WRITE_DELAY);
        if (statusSensorQueue != pdTRUE)
        {
            printf("%s.Sensor queue.Inserting:%d. Error sending data to queue!\n", sensor->name, sensorValue);
        }
        else
        {
            //printf("%s.Sensor queue.Inserting:%d\n", sensor->name, sensorValue);
        }

        vTaskDelay(pdMS_TO_TICKS(sensor->readingInterval));
    } //while

    vTaskDelete(NULL);
} //sensorTask

static void filterTask(void *args)
{
    Sensor_t *sensor = (Sensor_t *)args;
    int32_t newSensorValue;
    time_t timestampNewSensorValue;
    float weightedMean = 0;
    SensorData_t newSensorData;
    FilterData_t filterData;
    QueueHandle_t sensorQueue = sensor->sensorQueue;
    QueueHandle_t filterQueue = sensor->filterQueue;
    BaseType_t statusSensorQueue;

    //printf("\n-----------------------------------------\n");
    while (1)
    {
        statusSensorQueue = xQueueReceive(sensorQueue, &newSensorData, READ_DELAY);
        if (statusSensorQueue == pdTRUE)
        {
            newSensorValue = newSensorData.value;
            timestampNewSensorValue = newSensorData.timestamp;
            //printf("%s.Sensor queue.Getting:%d\n", sensor->name, newSensorValue);

            insertValue(newSensorValue, &sensor->filterCount, sensor->name);

            //Weighted mean
            if (sensor->filterCount.aforo < sensor->filterCount.aforoMax)
            {
                sensor->filterCount.aforo++;
            }
            else
            {
                weightedMean = getWeightedMean(sensor->filterCount, weightingCoefficients, sensor->name);

                filterData.valueOriginal = newSensorValue;
                filterData.timestamp = timestampNewSensorValue;
                filterData.valueWeighted = weightedMean;
                strcpy(filterData.taskName, sensor->filterTaskName);

                statusSensorQueue = xQueueSendToBack(filterQueue, &filterData, WRITE_DELAY);
                if (statusSensorQueue != pdTRUE)
                {
                    printf("%s.Filter queue.Inserting:%g. Error sending data to queue!\n", sensor->name, weightedMean);
                }
                else
                {
                    //printf("%s.Filter queue.Inserting:%g\n", sensor->name, weightedMean);
                }
            }
        }
        else
        {
            printf("%s.Sensor queue.Empty!\n", sensor->name);
        }

        vTaskDelay(pdMS_TO_TICKS(sensor->readingInterval));
    } //while
    vTaskDelete(NULL);
} //filterTask

static void controllerTask(void *args)
{
    QueueSetMemberHandle_t readingQueue;
    BaseType_t statusFilterQueue;
    FilterData_t filterData;
    int statisticsCounter = 2;
    float stadisticsCyclesDelay = STADISTICS_INTERVAL / CONTROLLER_INTERVAL;

    while (1)
    {
        readingQueue = xQueueSelectFromSet(controllerQueues, READ_DELAY);
        for (int i = 0; i < NUMBER_SENSORS; i++)
        {
            if (readingQueue == filterQueues[i])
            {
                statusFilterQueue = xQueueReceive(readingQueue, &filterData, READ_DELAY);
                if (statusFilterQueue == pdTRUE)
                {
                    printf("%s.Controller queue.Getting:%g\n", sensors[i].name, filterData.valueWeighted);
                    printf("%s - %g - %s", filterData.taskName, filterData.valueWeighted, ctime(&filterData.timestamp));
                }
            }
        }

        statisticsCounter++;
        if (statisticsCounter >= stadisticsCyclesDelay)
        {
            printf("STADISTICS_INTERVAL/CONTROLLER_INTERVAL: %i/%i = %g\n", STADISTICS_INTERVAL, CONTROLLER_INTERVAL, stadisticsCyclesDelay);
            showStadistics();
            statisticsCounter = 2;
        }

        vTaskDelay(pdMS_TO_TICKS(CONTROLLER_INTERVAL));
    } //while
    vTaskDelete(NULL);
} //controllerTask

/*---------------------------------------------------------------------------*/
// APP_MAIN
void app_main(void)
{
    init();

#ifdef CONFIG_READING_INTERVAL_SENSOR_0
    sensors[0].readingInterval = CONFIG_READING_INTERVAL_SENSOR_0;
#else
    sensors[0].readingInterval = READ_INTERVAL;
#endif

#ifdef CONFIG_READING_INTERVAL_SENSOR_1
    sensors[1].readingInterval = CONFIG_READING_INTERVAL_SENSOR_1;
#else
    sensors[1].readingInterval = READ_INTERVAL;
#endif

    controllerQueues = xQueueCreateSet(NUM_ELEM_FILTER_QUEUE * NUMBER_SENSORS);

    for (int i = 0; i < NUMBER_SENSORS; i++)
    {
        sensorQueues[i] = xQueueCreate(NUM_ELEM_SENSOR_QUEUE, sizeof(SensorData_t));
        filterQueues[i] = xQueueCreate(NUM_ELEM_FILTER_QUEUE, sizeof(FilterData_t));
        xQueueAddToSet(filterQueues[i], controllerQueues);

        if (sensorQueues[i] != NULL && filterQueues[i] != NULL)
        {
            sensors[i].sensorQueue = sensorQueues[i];
            sensors[i].filterQueue = filterQueues[i];
            strcpy(taskName, "");
            strcat(taskName, prefixReadTask);
            strcat(taskName, "_");
            strcat(taskName, prefixSensor);
            strcat(taskName, "_");
            snprintf(numeroEnString, 21, "%d", i);
            strcat(taskName, numeroEnString);
            xTaskCreate(&sensorTask, taskName, 3072, &sensors[i], PRIORITY_SENSOR_TASK, NULL);

            strcpy(taskName, "");
            strcat(taskName, prefixFilterTask);
            strcat(taskName, "_");
            strcat(taskName, prefixSensor);
            strcat(taskName, "_");
            snprintf(numeroEnString, 21, "%d", i);
            strcat(taskName, numeroEnString);
            strcpy(sensors[i].filterTaskName, taskName);
            xTaskCreate(&filterTask, taskName, 3072, &sensors[i], PRIORITY_FILTER_TASK, NULL);
        }
    }

    strcpy(taskName, "");
    strcat(taskName, prefixControllerTask);
    xTaskCreate(&controllerTask, taskName, 3072, NULL, PRIORITY_CONTROLLER_TASK, NULL);
} //app_main

/*---------------------------------------------------------------------------*/
