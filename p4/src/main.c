#include "customTypes.h"


esp_timer_handle_t periodic_timer_sensor_temp;

SemaphoreHandle_t semaphore_print = NULL;

/**
 * i2c master driver initialization
 */
static esp_err_t config_driver_master(void) {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO, 
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ, 
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * Convert Farenheit to Celsius according to page 22 - Si7021 doc
 */
float farenheitToCelsius(uint16_t *temp_farenheit){
    float celsius = 0;
    celsius = (175.72 * (*temp_farenheit))/ 65536 - 46.85;
    return celsius;
}

/**
 * Read Si7021 - A20 sensor data (humidity and temperature)
 */
static esp_err_t read_i2c_master_sensor(i2c_port_t i2c_num, uint8_t *data_temp_1, uint8_t *data_temp_2)
{
    int ret;
    // Write: ask for data
    printf("WRITING\n");
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    //TODO: revisar las direcciones del sensor y los comandos
    i2c_master_write_byte(cmd, SI7021_SENSOR_ADDR << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SI7021_CMD_START, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(30 / portTICK_RATE_MS);

    printf("READING\n");
    // Read: get data
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SI7021_SENSOR_ADDR << 1 | I2C_MASTER_READ, ACK_VAL);
    i2c_master_read_byte(cmd, data_temp_1, ACK_VAL);
    i2c_master_read_byte(cmd, data_temp_2, NACK_VAL);

    printf("byte 1: %02x\n", *data_temp_1);
    printf("byte 2: %02x\n", *data_temp_2);

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * Timer Si7021 - A20 sensor callback
 */
static void sensor_temp_timer_callback(void *args)
{
    uint8_t data_temp_1, data_temp_2;
    int ret = read_i2c_master_sensor(I2C_MASTER_NUM, &data_temp_1, &data_temp_2);
    xSemaphoreTake(semaphore_print, portMAX_DELAY);
    if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "I2C Timeout");
    } else if (ret == ESP_OK) {
        printf("*******************\n");
        printf("data_temperature - byte 1: %02x\n", data_temp_1);
        printf("data_temperature - byte 2: %02x\n", data_temp_2);
        
        //TODO: revisar las direcciones del sensor y como obtener un int16_t de la temperatura de 2bytes
        uint16_t temp = ((uint16_t)data_temp_2 << 8) | data_temp_1;
        printf("data_temperature - farenheit: %02x\n", temp);
        farenheitToCelsius(&temp);
        printf("data_temperature - celsius: %02x\n", temp);
        printf("*******************\n");
    } else {
        ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
    }
    xSemaphoreGive(semaphore_print);
}

void app_main() {
    /* -------------------------  INIT I2C DRIVER  ----------------------------*/
    ESP_ERROR_CHECK(config_driver_master());
    ESP_LOGI(TAG, "I2C master driver initialized successfully");

    /* --------------------  Si7021 - A20 SENSOR TIMER  -----------------------*/
    semaphore_print = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Configuring Si7021 - A20 sensor to print data each (%d) seconds", CONFIG_MILLIS_TO_PRINT / 1000);
        const esp_timer_create_args_t periodic_sensor_temp_timer_args = {
            .callback = &sensor_temp_timer_callback,
            .name = "periodic"};
    esp_timer_create(&periodic_sensor_temp_timer_args, &periodic_timer_sensor_temp);
    esp_timer_start_periodic(periodic_timer_sensor_temp, CONFIG_MILLIS_TO_PRINT*1000);

}