#include "customTypes.h"

esp_timer_handle_t periodic_timer_sensor_hall;
esp_timer_handle_t periodic_timer_reader;
int value_sensor = 0;

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
int redirect_log(const char * fmt, va_list ap) {

    ESP_LOGD(TAG, "Configuration log");

    FILE *f = fopen("/spiflash/log.txt", "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return 0;
    }
    fprintf(f, fmt);
    fclose(f);
    
    printf(fmt);

	return 1;
}


/**
 * Send machine status
 */
static void send_machine_state(void *args)
{
    enum machine_status status = *(enum machine_status *)args;
    if (xQueueSendFromISR(queueOut, &status, 200) != pdTRUE) {
        ESP_LOGE(TAG, "ERROR: Could not put item on delay queue.");
    }
}

/**
 * Sensor hall handler
 */
static void sensor_hall_timer_callback(void *args)
{
    // Reading voltage on ADC1 channel 0 (GPIO 36):
    adc1_config_width(ADC_WIDTH_BIT_12);
    value_sensor = hall_sensor_read();
    enum machine_status status = WRITE_LOG;
    send_machine_state(&status);
}

/**
 * Reader handler
 */
static void reader_timer_callback(void *args)
{
    enum machine_status status = READ_LOG;
    send_machine_state(&status);
}

static void status_handler_task(void *args)
{
    FILE *f;
    
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle));
    //esp_log_set_vprintf(redirect_log);

    // handle events for different status
    enum machine_status status;
    while(1) {
        if(xQueueReceive(queueOut, &(status) , (TickType_t)  300) == pdTRUE) {
            ESP_LOGD(TAG, "Status triggered: %s", machine_status_string[status]);
            switch (status)
            {
                case WRITE_LOG:
                ESP_LOGI(TAG, "Writting value %d", value_sensor);   
                f = fopen("/spiflash/log.txt", APPEND_FILE);
                if (f == NULL) {
                    ESP_LOGE(TAG, "Failed to open file for writing");
                }
                char str[40];
                sprintf(str, "Read value: %d \n", value_sensor);
                fprintf(f, str);
                fclose(f);  

                break;

                case READ_LOG:
                ESP_LOGI(TAG, "Reading values from file");
                //stop timers
                esp_timer_stop(periodic_timer_sensor_hall);
                esp_timer_stop(periodic_timer_reader);
                //read from logs.txt file
                f = fopen("/spiflash/log.txt", READ_FILE);
                if (f == NULL) {
                    ESP_LOGE(TAG, "Failed to open file for reading");
                    return;
                }

                int i = 0;
                int bufferLength = 255;
                char buffer[bufferLength];

                while (fgets(buffer, bufferLength, f) && i < NUM_LOGS_READ) {
                    printf("Read : '%s' from file: 'log.txt' \n", buffer);
                    i++;
                }
                fclose(f);

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
    queueOut = xQueueCreate( 10, sizeof( enum machine_status ));
    xTaskCreate(&status_handler_task, "status_machine_handler_task", 3072, NULL, PRIORITY_STATUS_HANDLER_TASK, NULL);


    /* -------------------------  SENSOR HALL  -------------------------------*/  
    const esp_timer_create_args_t periodic_hall_timer_args = {
        .callback = &sensor_hall_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_hall_timer_args, &periodic_timer_sensor_hall);
    esp_timer_start_periodic(periodic_timer_sensor_hall, MICROS_PERIODIC_SENSOR_HALL);

    /* -------------------------  READER  -------------------------------*/ 
    const esp_timer_create_args_t one_shot_reader_timer_args = {
        .callback = &reader_timer_callback,
        .name = "one-shot"};
    esp_timer_create(&one_shot_reader_timer_args, &periodic_timer_reader);
    esp_timer_start_periodic(periodic_timer_reader, MICROS_CHRONO_READER); 
}

