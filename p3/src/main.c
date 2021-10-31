#include "customTypes.h"

esp_timer_handle_t periodic_timer_chrono;
esp_timer_handle_t periodic_timer_sensor_hall;
static bool s_pad_activated[TOUCH_PAD_MAX];

QueueHandle_t queueOut;

/**
 * Send machine status
 */
static void send_machine_state(void *args)
{
    enum machine_status status = *(enum machine_status *)args;
    if (xQueueSend(queueOut, &status, 200) != pdTRUE) {
        ESP_LOGE(TAG, "ERROR: Could not put item on delay queue.");
    }
}

/**
 * Read values sensed at T0 touch pad 
 */
static void touchpad_read_task(void *args)
{
    ESP_LOGI(TAG, "Touch Sensor interrupt mode activated");

    while (1)
    {
        touch_pad_intr_enable();
        //Use pin T0 (pin GPIO4) from SensorTouch to start/stop chronometer functionality
        if (s_pad_activated[TOUCH_PAD_PIN_0] == true) {
            enum machine_status status = START_STOP_TIMER;
            send_machine_state(&status);
            // Wait a while for the pad being released
            vTaskDelay(200 / portTICK_PERIOD_MS);
            // Clear information on pad activation
            s_pad_activated[TOUCH_PAD_PIN_0] = false;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

/**
 * Handle an interrupt triggered when a pad is touched.
 */
static void rtc_intr_touch_pad(void *args)
{
    uint32_t pad_intr = touch_pad_get_status();
    //clear interrupt
    touch_pad_clear_status();
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        if ((pad_intr >> i) & 0x01) {
            s_pad_activated[i] = true;
        }
    }
}

/**
 * Before reading touch pad, we need to initialize the RTC IO.
 */
static void custom_touch_pad_init(void)
{
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        //init RTC IO and mode for touch pad.
        touch_pad_config(i, TOUCH_THRESH_NO_USE);
    }
}

/**
 * Set thresholds to 2/3 regarding to the original value
 */
static void set_threshold_touch_pad(void)
{

    uint16_t touch_value;
    for (int i = 0; i < TOUCH_PAD_MAX; i++)
    {
        touch_pad_read_filtered(i, &touch_value);
        ESP_LOGD(TAG, "Test init: touch pad [%d] val is %d", i, touch_value);
        ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * 2 / 3));
    }
}

/**
 * Timer callback handler
 */
static void chronometer_timer_callback(void *args)
{
    enum machine_status status = COUNT_TIMER;
    send_machine_state(&status);    
}

/**
 * Sensor hall handler
 */
static void sensor_hall_timer_callback(void *args)
{
    // Reading voltage on ADC1 channel 0 (GPIO 36):
    adc1_config_width(ADC_WIDTH_BIT_12);
    int value = hall_sensor_read();
    if(value <= SENSOR_HALL_THRESHOLD) {
        ESP_LOGD(TAG, "Sensor hall value: %d", value);
        enum machine_status status = RESET_TIMER;
        send_machine_state(&status);    
    }
}

static void status_handler_task(void *args)
{
    // handle events for different behaviors
    enum machine_status status;
    bool isTimerActive = true;
    int countTimer = 0;
    while(1) {
        if(xQueueReceive(queueOut, &(status) , (TickType_t)  300) == pdTRUE) {
            ESP_LOGD(TAG, "Status triggered: %s", machine_status_string[status]);
            switch (status)
            {
                case COUNT_TIMER:
                if (isTimerActive)
                {
                    countTimer++;
                }
                ESP_LOGI(TAG, "Chronometer (%02d:%02d)", countTimer / 60, countTimer % 60);
                break;

                case START_STOP_TIMER:
                isTimerActive = !isTimerActive;
                if(isTimerActive) {
                    esp_timer_start_periodic(periodic_timer_chrono, MICROS_PERIODIC_CHRONOMETER);
                    ESP_LOGI(TAG, "T%d activated, chronometer started", TOUCH_PAD_PIN_0);
                } else {
                    status = START_STOP_TIMER;
                    ESP_LOGI(TAG, "T%d activated, chronometer stopped", TOUCH_PAD_PIN_0);
                }
                break;

                case RESET_TIMER:
                countTimer = 0;
                isTimerActive = false;
                esp_timer_stop(periodic_timer_chrono);
                ESP_LOGI(TAG, "Chronometer reset");
                break;
                default:
                if (isTimerActive)
                {
                    countTimer++;
                }
                ESP_LOGI(TAG, "Chronometer (%02d:%02d)", countTimer / 60, countTimer % 60);
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);  
}

void app_main(void)
{
    /* ---------------------  STATUS MACHINE TASK  --------------------------*/   
    /**
     * 'machine_status' is an enum type defining all different states managed
     */
    queueOut = xQueueCreate( 10, sizeof( enum machine_status ));
    xTaskCreate(&status_handler_task, "status_machine_handler_task", 3072, NULL, PRIORITY_STATUS_HANDLER_TASK, NULL);

    /* -------------------------  TOUCH PAD  -------------------------------*/   
    ESP_LOGI(TAG, "Initializing touch pad");
    ESP_ERROR_CHECK(touch_pad_init());
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    // Set reference voltage for charging/discharging
    // High reference valtage will be 2.7V - 1V = 1.7V
    // Low reference voltage will be 0.5
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    custom_touch_pad_init();
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    // Set threshold
    set_threshold_touch_pad();
    
    // Register touch interrupt ISR
    touch_pad_isr_register(&rtc_intr_touch_pad, NULL);
    xTaskCreate(&touchpad_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL);

    /* -------------------------  SENSOR HALL  -------------------------------*/  
    const esp_timer_create_args_t periodic_hall_timer_args = {
        .callback = &sensor_hall_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_hall_timer_args, &periodic_timer_sensor_hall);
    esp_timer_start_periodic(periodic_timer_sensor_hall, MICROS_PERIODIC_SENSOR_HALL);


    /* -------------------------  CHRONOMETER  -------------------------------*/  
    const esp_timer_create_args_t periodic_chrono_timer_args = {
        .callback = &chronometer_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_chrono_timer_args, &periodic_timer_chrono);
    esp_timer_start_periodic(periodic_timer_chrono, MICROS_PERIODIC_CHRONOMETER);

    // Delete tasks
    vTaskDelete(NULL);
}

