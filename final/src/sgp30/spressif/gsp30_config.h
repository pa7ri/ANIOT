#define I2C_MASTER_NUM CONFIG_I2C_MASTER_NUM                // select 0 or 1 availables
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA             // 21 default GPIO21 via menuconfig
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL             // 22 default GPIO22 via menuconfig
#define I2C_MASTER_FREQ_HZ 50000                            // I2C master clock frequency. Max 1MHz
#define I2C_MASTER_RX_BUF_DISABLE 0                         // Receiving buffer size. Only slave mode will use this value, it is ignored in master mode. 
#define I2C_MASTER_TX_BUF_DISABLE 0                         // Sending buffer size. Only slave mode will use this value, it is ignored in master mode. 