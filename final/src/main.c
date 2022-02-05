#include "customTypes.h"

esp_timer_handle_t periodic_co2_timer_sensor;
esp_timer_handle_t periodic_send_data_timer;

int value_sensor = 0;
bool is_first_time = true;

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
char *base_path = "/spiflash";    //Base path for FAT
const esp_vfs_fat_mount_config_t mount_config = {
    .max_files = 2,
    .format_if_mount_failed = true,
    .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

QueueHandle_t queueOut;

/**
 * Redirect logs to file
 */
int redirect_log(const char * message, va_list args) {
    FILE *f;
    printf("Redirecting log value \n");
    if(is_first_time) {
        is_first_time = false;
        f = fopen("/spiflash/log.txt", WRITE_FILE);
    } else {
        f = fopen("/spiflash/log.txt", APPEND_FILE);
    }
    if (f == NULL) {
        printf("Error openning file \n");
        return -1;
    }
    vfprintf(f, message, args);
    fclose(f);

	return 0;
}


/**
 * Send machine status
 */
static void send_machine_state(void *args)
{
    enum machine_status_ID status = *(enum machine_status_ID *)args;
    if (xQueueSendFromISR(queueOut, &status, 200) != pdTRUE) {
        ESP_LOGE(TAG, "ERROR: Could not put item on delay queue.");
    }
}

/**
 * Convert raw signal to cms according to infrared sensor documentation
 */
static double raw_to_cms(int valueRaw)
{
    return (double) FUNCTION_CMS_VAR_A * pow(valueRaw, FUNCTION_CMS_VAR_B);
}


/**
 * Sensor CO2 handler
 */
static void sensor_co2_timer_callback(void *args)
{
    uint8_t data_co2_1, data_co2_2;
    int ret = read_i2c_master_sensor(I2C_MASTER_NUM, &data_co2_1, &data_co2_2);
    if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "I2C Timeout");
    } else if (ret == ESP_OK) {
        uint16_t co2Value = ((uint16_t)data_co2_1 << 8) + data_co2_2;

        machine_status machineStatus;
        machineStatus.sensorData.co2 = co2Value;
        machineStatus.statusId = READ_CO2;
        send_machine_state(&machineStatus);
    } else {
        ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
    } 
}

static void sensor_co2_send_data_callback(void *args)
{
    machine_status machineStatus;
    machineStatus.statusId = SEND_CO2;
    send_machine_state(&machineStatus);
}


/**
 * Read SGP30 - CO2 sensor data
 */
static esp_err_t read_i2c_master_sensor(i2c_port_t i2c_num, uint8_t *data_co2_1, uint8_t *data_co2_2)
{
    int ret;
    // Write: ask for data
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x40 << 1) | I2C_MASTER_WRITE, ACK_VAL);
    i2c_master_write_byte(cmd, 0xF3, ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(30 / portTICK_RATE_MS);

    // Read: get data
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x40 << 1) | I2C_MASTER_READ, ACK_VAL);
    i2c_master_read_byte(cmd, data_co2_1, ACK_VAL);
    i2c_master_read_byte(cmd, data_co2_2, NACK_VAL);

    printf("byte 1: %02x\n", *data_co2_1);
    printf("byte 2: %02x\n", *data_co2_2);

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

float computeDistanceMean(float sensorInfrarredValues[]) {
    float mean = 0;
    for(int i = 0; i < CONFIG_N_SAMPLES; i++) {
        mean += sensorInfrarredValues[i];
    }
    return mean/CONFIG_N_SAMPLES;
}

int16_t computeCO2Mean(int16_t sensorCO2Values[]) {
    int16_t mean = 0;
    for(int i = 0; i < CONFIG_N_SAMPLES; i++) {
        mean += sensorCO2Values[i];
    }
    return (int16_t)mean/CONFIG_N_SAMPLES;
}

static void status_handler_task(void *args)
{

    //TODO: create a queue to store values for CO2 sensor and infrarred sensor
    int16_t sensorCO2Values[CONFIG_N_SAMPLES];
    float sensorInfrarredValues[CONFIG_N_SAMPLES];
    int counterCO2 = 0;
    int counterInfrarred = 0;
    /* ---------------------  HANDLE EVENT STATUS --------------------------*/ 

    machine_status machineStatus;
    while(1) {
        if(xQueueReceive(queueOut, &(machineStatus) , (TickType_t)  300) == pdTRUE) {
            printf("Status triggered: %s \n", machine_status_string[machineStatus.statusId]);
            switch (machineStatus.statusId) 
            {
                case READ_CO2:
                    //TODO: read data from sensor and store in a queue
                    sensorCO2Values[counterCO2] = machineStatus.sensorData.co2;
                    counterCO2++;
                break;
                case SEND_CO2:
                    //TODO: get average of values in the queue, send data and clean the queue
                    
                    float co2Mean = computeCO2Mean(sensorCO2Values);
                    printf("CO2 status : %f ml/s \n", co2Mean);
                    counterCO2 = 0;
                    memset(sensorCO2Values, 0, CONFIG_N_SAMPLES*sizeof(int16_t));

                break;
                case READ_INFRARRED:
                    //TODO: store in a queue
                    float valueCms = raw_to_cms(machineStatus.sensorData.distance);
                    sensorInfrarredValues[counterInfrarred] = valueCms;
                    counterInfrarred++;
                break;
                case SEND_INFRARRED:
                    //TODO: get average of values in the queue, send data and clean the queue
                    float distanceMean = computeDistanceMean(sensorInfrarredValues);
                    if(distanceMean > OPEN_WINDOW_THRESHOLD) {
                        printf("Window status : CLOSED \n");
                    } else {
                        printf("Window status : OPEN \n");
                    }
                    counterInfrarred = 0;
                    memset(sensorInfrarredValues, 0, CONFIG_N_SAMPLES*sizeof(float));
                break;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);  

    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));
}

void app_main(void)
{
    /* ---------------------  STATUS MACHINE TASK  --------------------------*/   
    /**
     * 'machine_status' is an enum type defining all different states managed
     */
    queueOut = xQueueCreate( 10, sizeof( machine_status ));
    xTaskCreate(&status_handler_task, "status_machine_handler_task", 3072, NULL, PRIORITY_STATUS_HANDLER_TASK, NULL);


    /* -------------------------  SENSOR READ CO2 TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_sensor_co2_timer_args = {
        .callback = &sensor_co2_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_sensor_co2_timer_args, &periodic_co2_timer_sensor);
    esp_timer_start_periodic(periodic_co2_timer_sensor, CONFIG_SAMPLE_FREQ *1000);

    /* -------------------------  SEND DATA TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_send_data_timer_args = {
        .callback = &sensor_co2_send_data_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_send_data_timer_args, &periodic_send_data_timer);
    esp_timer_start_periodic(periodic_send_data_timer, CONFIG_SAMPLE_FREQ*CONFIG_N_SAMPLES *1000);
}
