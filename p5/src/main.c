#include "customTypes.h"


esp_timer_handle_t periodic_timer_sensor_read;

int read_count = 0;
bool sleep_mode_active = false;

static RTC_DATA_ATTR struct timeval sleep_enter_time;

/**
 * i2c master driver initialization
 */
static esp_err_t config_driver_master(void) 
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO, 
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ, 
    };
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * Read Si7021 - A20 sensor data (humidity and temperature)
 */
static esp_err_t read_i2c_master_sensor(i2c_port_t i2c_num, uint8_t *data_temp_1, uint8_t *data_temp_2)
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
    i2c_master_read_byte(cmd, data_temp_1, ACK_VAL);
    i2c_master_read_byte(cmd, data_temp_2, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * Get wakeup cause
 */
static char* get_wakeup_cause()
{
    char* wakeup_reason;
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:
            wakeup_reason = "timer";
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            wakeup_reason = "touchpad";
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            wakeup_reason = "ext0";
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            wakeup_reason = "ext1";
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            wakeup_reason = "ulp";
            break;
        case ESP_SLEEP_WAKEUP_UART: // Light mode only
            wakeup_reason = "uart";
            break;
        case ESP_SLEEP_WAKEUP_GPIO: // Light mode only
            wakeup_reason = "pin";
            break;
        default:
            wakeup_reason = "other";
            break;
    }

    return wakeup_reason;
}

/**
 * Start light sleep mode with 10 secs timer wakeup
 */
static void start_light_mode() {
    ESP_LOGI(TAG, "Entering light sleep");
    int64_t t_before_us = esp_timer_get_time();
    esp_sleep_enable_timer_wakeup(TIMER_WAKE_UP_SLEEP_MODE);
    esp_light_sleep_start();
    int64_t t_after_us = esp_timer_get_time();
    char* wakeup_cause = get_wakeup_cause();
    ESP_LOGI(TAG, "Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
        wakeup_cause, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
}

/**
 * Start deep sleep mode with 10 secs timer wakeup
 */
static void start_deep_mode() {
    ESP_LOGI(TAG, "Entering deep sleep");
    gettimeofday(&sleep_enter_time, NULL);    
    esp_sleep_enable_timer_wakeup(TIMER_WAKE_UP_SLEEP_MODE);
    esp_deep_sleep_start();
}

/**
 * Timer Si7021 - A20 sensor task
 */
static void sensor_temp_task(void *args)
{
    while (1) {
        if(read_count > MAX_READ_COUNT && !sleep_mode_active) {
            sleep_mode_active = true;
            /*-------- Start light sleep mode -------*/
            //start_light_mode();
            /*-------- Start deep sleep mode -------*/
            start_deep_mode();
        } else {
            uint8_t data_temp_1, data_temp_2;
            int ret = read_i2c_master_sensor(I2C_MASTER_NUM, &data_temp_1, &data_temp_2);
            if (ret == ESP_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "I2C Timeout");
            } else if (ret == ESP_OK) {
                uint16_t temp = ((uint16_t)data_temp_1 << 8) + data_temp_2;
                ESP_LOGI(TAG, "Sensor data: %02x", temp);
            } else {
                ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
            }        
            read_count++;
        }
        vTaskDelay((CONFIG_MILLIS_TO_PRINT - 1000) / portTICK_RATE_MS);
        // Remove 1000milis from write/read wait
        
    }
    vTaskDelete(NULL);
}


/**
 * Timer Si7021 - A20 sensor timer callback
 */
static void sensor_read_timer_callback(void *args)
{
    if(read_count > MAX_READ_COUNT && !sleep_mode_active) {
            sleep_mode_active = true;
            /*-------- Start light sleep mode -------*/
            //start_light_mode();
            /*-------- Start deep sleep mode -------*/
            start_deep_mode();
    } else {
            uint8_t data_temp_1, data_temp_2;
            int ret = read_i2c_master_sensor(I2C_MASTER_NUM, &data_temp_1, &data_temp_2);
            if (ret == ESP_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "I2C Timeout");
            } else if (ret == ESP_OK) {
                uint16_t temp = ((uint16_t)data_temp_1 << 8) + data_temp_2;
                ESP_LOGI(TAG, "Sensor data: %02x", temp);
            } else {
                ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
            }        
            read_count++;
        }
}

void app_main() {
    /* --------------------- GET CAUSE FOR DEEP SLEEP -------------------------*/
    struct timeval now;
    gettimeofday(&now, NULL);
    int64_t sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
    char* wakeup_cause = get_wakeup_cause();
    ESP_LOGI(TAG, "Returned from light sleep, reason: %s, slept for %lld ms\n",
        wakeup_cause, sleep_time_ms);



    /* -------------------------  INIT I2C DRIVER  ----------------------------*/
    ESP_ERROR_CHECK(config_driver_master());
    ESP_LOGI(TAG, "I2C master driver initialized successfully");

    /* --------------------  Si7021 - A20 SENSOR TASK  -----------------------
    xTaskCreatePinnedToCore(sensor_temp_task, "i2c_temp_task_0", 3072, NULL, PRIORITY_TASK, NULL,0);*/
    /* --------------------  Si7021 - A20 SENSOR TIMER  -----------------------*/
    const esp_timer_create_args_t periodic_sleep_sensor_read_args = {
            .callback = &sensor_read_timer_callback,
            .name = "periodic"};
        esp_timer_create(&periodic_sleep_sensor_read_args, &periodic_timer_sensor_read);
        esp_timer_start_periodic(periodic_timer_sensor_read, CONFIG_MILLIS_TO_PRINT*1000-1000000);

}