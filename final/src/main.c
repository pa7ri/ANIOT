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
static void sensor_send_data_callback(void *args)
{
    enum machine_status status = SEND_CO2;
    send_machine_state(&status);
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
                    //TODO: read data from sensor and store in a queue
                    uint8_t data_co2_1, data_co2_2;
                    int ret = read_i2c_master_sensor(I2C_MASTER_NUM, &data_co2_1, &data_co2_1);
                break;
                case SEND_CO2:
                    //TODO: get average of values in the queue, send data and clean the queue
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


    /* -------------------------  SENSOR READ CO2 TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_sensor_co2_timer_args = {
        .callback = &sensor_co2_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_sensor_co2_timer_args, &periodic_co2_timer_sensor);
    esp_timer_start_periodic(periodic_co2_timer_sensor, CONFIG_SAMPLE_FREQ *1000);

    /* -------------------------  SEND DATA TIMER  -------------------------------*/  
    const esp_timer_create_args_t periodic_send_data_timer_args = {
        .callback = &sensor_send_data_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_send_data_timer_args, &periodic_send_data_timer);
    esp_timer_start_periodic(periodic_send_data_timer, CONFIG_SAMPLE_FREQ*CONFIG_N_SAMPLES *1000);
}

