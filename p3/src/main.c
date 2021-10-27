#include "customTypes.h"

esp_timer_handle_t periodic_timer_chrono;
esp_timer_handle_t periodic_timer_sensor_hall;
static bool s_pad_activated[TOUCH_PAD_MAX];
bool isTimerActive = true;
int countTimer = 0;

/*
  Read values sensed at T0 touch pad 
 */
static void touchpad_read_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Touch Sensor interrupt mode activated");

    while (1)
    {
        touch_pad_intr_enable();
        //Use pin T0 (pin GPIO4) from SensorTouch to start/stop chronometer functionality
        if (s_pad_activated[TOUCH_PAD_PIN_0] == true) {
            //TODO: replace flag with start/stop event
            isTimerActive = !isTimerActive;
            if(isTimerActive) {
                ESP_LOGI(TAG, "T%d activated, chronometer started", TOUCH_PAD_PIN_0);
            } else {
                ESP_LOGI(TAG, "T%d activated, chronometer stopped", TOUCH_PAD_PIN_0);
            }
            // Wait a while for the pad being released
            vTaskDelay(200 / portTICK_PERIOD_MS);
            // Clear information on pad activation
            s_pad_activated[TOUCH_PAD_PIN_0] = false;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

/*
  Handle an interrupt triggered when a pad is touched.
 */
static void rtc_intr_touch_pad(void *arg)
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

/*
 * Before reading touch pad, we need to initialize the RTC IO.
 */
static void custom_touch_pad_init(void)
{
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        //init RTC IO and mode for touch pad.
        touch_pad_config(i, TOUCH_THRESH_NO_USE);
    }
}

/*
  Set thresholds to 2/3 regarding to the original value
 */
static void set_threshold_touch_pad(void)
{

    uint16_t touch_value;
    for (int i = 0; i < TOUCH_PAD_MAX; i++)
    {
        touch_pad_read_filtered(i, &touch_value);
        ESP_LOGI(TAG, "test init: touch pad [%d] val is %d", i, touch_value);
        ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * 2 / 3));
    }
}

/*
  Reset chronometer to 0 and stop timer.
 */
static void reset_timer()
{
    countTimer = 0;
    esp_timer_stop(periodic_timer_chrono);
}


/*
  Timer callback handler
 */
static void chronometer_timer_callback(void *args)
{
    //TODO: replace with counter event
    if (isTimerActive)
    {
        countTimer++;
    }
    ESP_LOGI(TAG, "Chronometer (%02d:%02d)", countTimer / 60, countTimer % 60);
}

/*
  Sensor hall handler
 */
static void sensor_hall_timer_callback(void *args)
{
    // Reading voltage on ADC1 channel 0 (GPIO 36):
    adc1_config_width(ADC_WIDTH_BIT_12);
    int value = hall_sensor_read();
    // printf("Sensor hall value: %d \n", value);
    if(value <= SENSOR_HALL_THRESHOLD) {
        reset_timer(); //TODO: replace with reset event
    }
}

void app_main(void)
{

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

