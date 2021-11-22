/* Heap Task Tracking Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "tipos_2.h"
QueueHandle_t queueOut;

static void sensor_task(void *args)
{
    // Receive n ticks as argument 
    sensor_t *sensor = (sensor_t *) args;
    const TickType_t taskDelay = sensor->millis / portTICK_PERIOD_MS;

    // Create a queue capable of containing 5 uint32_t values.
    QueueHandle_t xQueue;
    xQueue = xQueueCreate( 5, sizeof( sensor_data_t ) ); 
    sensor->queue = xQueue;
    
    // Send random data and queue 
    sensor_data_t sensorData;
    while(1) {
        sensorData.number = esp_random() % 100;
        sensorData.timestamp = time(NULL);
        printf("------ Generated data ----- \n Sensor ID : %d \n Random number: %d \n Timestamp: %ld \n", sensor->id, sensorData.number, (long) sensorData.timestamp);
        if (xQueueSend(sensor->queue, &sensorData, 200) != pdTRUE) {
            printf("ERROR: Could not put item on delay queue.");
        }
        vTaskDelay(taskDelay);
    }
    vTaskDelete(NULL);    
}

static void filter_task(void *args)
{
    sensor_t sensor = *(sensor_t *) args;
    int queue[MAX_SIZE];

    sensor_data_t sensorData;
    controller_t controllerData;
    while(1) {
        if(xQueueReceive(sensor.queue, &(sensorData) , (TickType_t)  300) == pdTRUE) {
            printf("------ Received data ----- \n Sensor ID : %d \n Data: %d \n", sensor.id, sensorData.number); 

            // Insert data
            queue[(sensor.count)%MAX_SIZE] = sensorData.number;
            sensor.count++;
            
            // If greater than 5, compute mean
            if(sensor.count >= MAX_SIZE) {
                int total = MEAN_WEIGHT_1 + MEAN_WEIGHT_2 + MEAN_WEIGHT_3 + MEAN_WEIGHT_4 + MEAN_WEIGHT_5;
                float mean = (queue[0]* MEAN_WEIGHT_1) + (queue[1]* MEAN_WEIGHT_2) + (queue[2]* MEAN_WEIGHT_3) + (queue[3]* MEAN_WEIGHT_4) + (queue[4]* MEAN_WEIGHT_5) / total;
                printf("------ Computed mean ----- \n Sensor ID : %d \n Mean: %f \n", 1, (float) mean); 

                //Send filtered data
                controllerData.sensorId = sensor.id;
                controllerData.taskId = sensor.tag;
                controllerData.data = mean;
                controllerData.timestamp = sensorData.timestamp;

                if (xQueueSend(queueOut, &controllerData, 200) != pdTRUE) {
                    printf("ERROR: Could not put item on delay queue.");
                }
            }      
        }
            vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}


static void controller_task(void *args)
{
    controller_t controllerData;
    while(1) {
        if(xQueueReceive(queueOut, &(controllerData) , (TickType_t)  300) == pdTRUE) {
            controllerData.sensorId, controllerData.taskId, controllerData.data, controllerData.timestamp); 
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);  
    
}


void app_main(void)
{
    srand(time(NULL));

    queueOut = xQueueCreate( 10, sizeof( controller_t ) ); 

    // Create sensor tasks to send data
    sensor_t sensor1;
    sensor1.count= 0;
    sensor1.id = ID_SENSOR_1;
    sensor1.tag = ID_TASK_1;
    sensor1.millis = MILLIS_DELAY_SENSOR_1;
    xTaskCreate(&sensor_task, "sensor_task_1", 3072, &sensor1, PRIORITY_SENSOR_TASK, NULL); 

    sensor_t sensor2;
    sensor2.count= 0;
    sensor2.id = ID_SENSOR_2;
    sensor1.tag = ID_TASK_2;
    sensor2.millis = MILLIS_DELAY_SENSOR_2;
    xTaskCreate(&sensor_task, "sensor_task_2", 3072, &sensor2, PRIORITY_SENSOR_TASK, NULL);


    // Create filter task for each sensor    
    xTaskCreate(&filter_task, "filter_task_1", 3072, &sensor1, PRIORITY_FILTER_TASK, NULL);
    xTaskCreate(&filter_task, "filter_task_2", 3072, &sensor2, PRIORITY_FILTER_TASK, NULL);


    // Create controller task for both sensors   
    xTaskCreate(&controller_task, "controller_task", 3072, NULL, PRIORITY_CONTROLLER_TASK, NULL);

    // Delete created task for security
    vTaskDelete(NULL);

}
