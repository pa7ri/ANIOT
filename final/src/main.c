#include "customTypes.h"
#include "sgp30/sgp30.h"

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
 * Send machine state
 */
static void send_machine_state(void *args)
{
    machine_state state = *(machine_state *)args;
    if (xQueueSendFromISR(queueOut, &state, 200) != pdTRUE) {
        ESP_LOGE(TAG, "ERROR: Could not put item on delay queue.");
    }
}


/**
 * Sensor CO2 read handler
 */
static void sensor_co2_timer_callback(void *args) {
    machine_state machineState;
    uint16_t tvoc;
    uint16_t co2;

    uint16_t tvoc_raw;
    uint16_t co2_raw;

    sgp30_measure_iaq_blocking_read(&tvoc, &co2);
    sgp30_measure_raw_blocking_read(&tvoc_raw, &co2_raw);

    // ESP_LOGI(TAG, "Read quality air tvoc: %i, co2: %i", tvoc, co2);  -- store data inside SPI FLASH file
    printf("Read quality air tvoc: %i, co2: %i\n", tvoc, co2);
    printf("Read quality air raw tvoc: %i, co2: %i\n", tvoc, co2);

    machineState.sensorData.tvoc = tvoc;
    machineState.sensorData.co2 = co2;
    machineState.stateId = READ_CO2;

    send_machine_state(&machineState);
}

static void sensor_send_data_callback(void *args) {
    machine_state machineState;
    machineState.stateId = SEND_CO2;
    send_machine_state(&machineState);
}

static double raw_to_cms(int valueRaw)
{
    return (double) FUNCTION_CMS_VAR_A * pow(valueRaw, FUNCTION_CMS_VAR_B);
}

/**
 * Compute mean for infrearred values
 */
float computeDistanceMean(float sensorInfrarredValues[]) {
    float mean = 0;
    for(int i = 0; i < CONFIG_N_SAMPLES; i++) {
        mean += sensorInfrarredValues[i];
    }
    return mean/CONFIG_N_SAMPLES;
}

/**
 * Compute mean for air quality measure sensor values
 */
float computeCO2Mean(uint16_t sensorCO2Values[]) {
    uint16_t mean = 0;
    for(int i = 0; i < CONFIG_N_SAMPLES; i++) {
        mean += sensorCO2Values[i];
    }
    return (float)mean/CONFIG_N_SAMPLES;
}

static void state_handler_task(void *args)
{    
    /* ---------------------  REDIRECT LOGS A SPI FLASH --------------------------*/  
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle));
    esp_log_set_vprintf(&redirect_log);

    /* ---- Define structures to handle the mean for measured values from sensors ----*/
    uint16_t sensorCO2Values[CONFIG_N_SAMPLES];
    uint16_t sensorTVOCValues[CONFIG_N_SAMPLES];
    float sensorInfrarredValues[CONFIG_N_SAMPLES];
    float co2Mean, tvocMean, valueCms, distanceMean;
    int counterCO2 = 0;
    int counterInfrarred = 0;

    // handle events for different state
    machine_state machineState;
    while(1) {
        if(xQueueReceive(queueOut, &(machineState) , (TickType_t)  300) == pdTRUE) {
            printf("State triggered: %s \n", machine_state_string[machineState.stateId]);
            switch (machineState.stateId) 
            {
                case READ_CO2:
                    sensorCO2Values[counterCO2] = machineState.sensorData.co2;
                    sensorTVOCValues[counterCO2] = machineState.sensorData.tvoc;
                    counterCO2++;
                break;
                case SEND_CO2:
                    co2Mean = computeCO2Mean(sensorCO2Values);
                    tvocMean = computeCO2Mean(sensorTVOCValues);
                    printf("Average CO2 value : %f ml/s \n", co2Mean);
                    printf("Average TVOC value : %f ml/s \n", tvocMean);
                    counterCO2 = 0;
                    memset(sensorCO2Values, 0, CONFIG_N_SAMPLES*sizeof(uint16_t));
                    memset(sensorTVOCValues, 0, CONFIG_N_SAMPLES*sizeof(uint16_t));
                break;
                case READ_INFRARRED:
                    valueCms = raw_to_cms(machineState.sensorData.distance);
                    sensorInfrarredValues[counterInfrarred] = valueCms;
                    counterInfrarred++;
                break;
                case SEND_INFRARRED:
                    distanceMean = computeDistanceMean(sensorInfrarredValues);
                    if(distanceMean > OPEN_WINDOW_THRESHOLD) {
                        printf("Window state : CLOSED \n");
                    } else {
                        printf("Window state : OPEN \n");
                    }
                    counterInfrarred = 0;
                    memset(sensorInfrarredValues, 0, CONFIG_N_SAMPLES*sizeof(float));
                break;
                default:
                    ESP_LOGW(TAG, "Unknown state received");
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
    /* ---------------------  STATE MACHINE TASK  --------------------------*/   
    /**
     * 'machine_state' is an enum type defining all different states managed
     */
    queueOut = xQueueCreate( 10, sizeof( machine_state ));
    xTaskCreate(&state_handler_task, "state_machine_handler_task", 3072, NULL, PRIORITY_STATE_HANDLER_TASK, NULL);

    //Init co2 sensor
    sensirion_i2c_init();
    sgp30_iaq_init();

    /* -------------------------  SENSOR READ CO2 TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_sensor_co2_timer_args = {
        .callback = &sensor_co2_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_sensor_co2_timer_args, &periodic_co2_timer_sensor);
    esp_timer_start_periodic(periodic_co2_timer_sensor, CONFIG_SAMPLE_FREQ * 1000);

    /* -------------------------  SEND DATA TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_send_data_timer_args = {
        .callback = &sensor_send_data_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_send_data_timer_args, &periodic_send_data_timer);
    esp_timer_start_periodic(periodic_send_data_timer, CONFIG_SAMPLE_FREQ * CONFIG_N_SAMPLES *1000);
}

