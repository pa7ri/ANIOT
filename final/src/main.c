#include "customTypes.h"

esp_timer_handle_t periodic_co2_timer_sensor;
esp_timer_handle_t periodic_infrarred_timer_sensor;
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
    enum machine_status status = *(enum machine_status *)args;
    if (xQueueSendFromISR(queueOut, &status, 200) != pdTRUE) {
        ESP_LOGE(TAG, "ERROR: Could not put item on delay queue.");
    }
}

/**
 * Sensor CO2 handler
 */
static void sensor_co2_timer_callback(void *args)
{
    // TODO
    enum machine_status status = READ_CO2;
    send_machine_state(&status);
}

/**
 * Sensor infrarred handler
 */
static void sensor_infrarred_timer_callback(void *args)
{
    // TODO
    enum machine_status status = READ_INFRARRED;
    send_machine_state(&status);
}

static void status_handler_task(void *args)
{
    FILE *f;
    const int bufferLength = 255;
    
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle));
    esp_log_set_vprintf(&redirect_log);

    // handle events for different status
    enum machine_status status;
    while(1) {
        if(xQueueReceive(queueOut, &(status) , (TickType_t)  300) == pdTRUE) {
            printf("Status triggered: %s \n", machine_status_string[status]);
            switch (status) 
            {
                case READ_CO2:
                    //store in file
                break;
                case READ_INFRARRED:
                    //store in file
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

    /* ---------------------  REDIRECT LOGS A SPI FLASH --------------------------*/  
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle));
    esp_log_set_vprintf(&redirect_log);

    /* ---------------------  STATUS MACHINE TASK  --------------------------*/   
    /**
     * 'machine_status' is an enum type defining all different states managed
     */
    queueOut = xQueueCreate( 10, sizeof( enum machine_status ));
    xTaskCreate(&status_handler_task, "status_machine_handler_task", 3072, NULL, PRIORITY_STATUS_HANDLER_TASK, NULL);


    /* -------------------------  SENSOR CO2 TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_sensor_co2_timer_args = {
        .callback = &sensor_co2_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_sensor_co2_timer_args, &periodic_co2_timer_sensor);
    esp_timer_start_periodic(periodic_co2_timer_sensor, CONFIG_SAMPLE_FREQ *1000);

    /* -------------------------  SENSOR INFRARRED TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_sensor_infrarred_timer_args = {
        .callback = &sensor_infrarred_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_sensor_infrarred_timer_args, &periodic_infrarred_timer_sensor);
    esp_timer_start_periodic(periodic_infrarred_timer_sensor, CONFIG_SAMPLE_FREQ *1000);
}

