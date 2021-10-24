#include "types.h"

esp_timer_handle_t periodic_timer;
bool isTimerActive = true;
int countTimer = 0;
int32_t time_since_boot = 0;

#define TOUCH_PAD_NO_CHANGE (-1)
#define TOUCH_THRESH_NO_USE (0)
#define TOUCH_FILTER_MODE_EN (1)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

/*
  Read values sensed at T0 and T9 touch pads and print out their values.
 */
static void touchpad_read_task(void *args)
{
    uint16_t touch_value;
    uint16_t touch_filter_value;
    printf("Touch Sensor filter mode read, the output format is: \nTouchpad num:[raw data, filtered data]\n\n");

    while (1)
    {
        //Use pin T0 (pin GPIO4) from SensorTouch to start counter functionality
        touch_pad_read_raw_data(0, &touch_value);
        touch_pad_read_filtered(0, &touch_filter_value);
        printf("T0:[%4d,%4d] \n", touch_value, touch_filter_value);

        if (touch_value <= 500 && !isTimerActive)
        { //5V
            isTimerActive = true;
            printf("Chronometer started \n");
            esp_timer_start_periodic(periodic_timer, MICROS_PER_SECOND);
        }

        //Use pin T9 (pin 32K_XP) from SensorTouch to stop counter functionality
        touch_pad_read_raw_data(9, &touch_value);
        touch_pad_read_filtered(9, &touch_filter_value);
        printf("T9:[%4d,%4d] \n", touch_value, touch_filter_value);
        if (touch_value <= 500)
        { //5V
            isTimerActive = false;
            printf("Chronometer stopped \n");
            //esp_timer_stop_periodic(periodic_timer);
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}


/*
  Reset chronometer to 0 and stop timer.
 */
static void reset_timer()
{
    countTimer = 0;
    esp_timer_stop(periodic_timer);
}

/*
  Read hall signal.
 */
static void hall_read_task(void* args) {
    while (1)
    {
        adc1_config_width(ADC_WIDTH_12Bit);
        int val = hall_sensor_read();
        if(val > 10) { //TODO: check values
            reset_timer();
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

/*
  Init configuration for T0 and T9 touch pads. Max = TOUCH_PAD_MAX (10).
 */
static void custom_touch_pad_init(void)
{
    touch_pad_config(0, TOUCH_THRESH_NO_USE);
    touch_pad_config(9, TOUCH_THRESH_NO_USE);
}


/*
  Timer callback handler
 */
static void second_timer_callback(void *args)
{
    if (isTimerActive)
    {
        countTimer++;
    }
    printf("Chronometer (%02d:%02d) \n", countTimer / 60, countTimer % 60);
    //int64_t time_since_boot = esp_timer_get_time();
}

void app_main(void)
{

    // Touch pad
    ESP_ERROR_CHECK(touch_pad_init());
    // Set reference voltage for charging/discharging
    // In this case, the high reference valtage will be 2.7V - 1V = 1.7V
    // The low reference voltage will be 0.5
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    custom_touch_pad_init();

    #if TOUCH_FILTER_MODE_EN
        touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    #endif

    xTaskCreate(&touchpad_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL);

    // Hall TO FIX!
    //xTaskCreate(&hall_read_task, "hall_read_task", 2048, NULL, 5, NULL);

    // Timer
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &second_timer_callback,
        .name = "periodic"};
    esp_timer_create(&periodic_timer_args, &periodic_timer);

    esp_timer_start_periodic(periodic_timer, MICROS_PER_SECOND);

    // Delete tasks
    vTaskDelete(NULL);
}
