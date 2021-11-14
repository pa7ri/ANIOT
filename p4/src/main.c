#include "customTypes.h"


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
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void app_main() {

    ESP_ERROR_CHECK(config_driver_master());
    ESP_LOGI(TAG, "I2C master driver initialized successfully");

    #if defined CONFIG_MILLIS_TO_PRINT
        ESP_LOGE(TAG, "Show temperature each (%d) seconds", CONFIG_MILLIS_TO_PRINT / 1000);
    #else
        ESP_LOGE(TAG, "No data");
    #endif
}