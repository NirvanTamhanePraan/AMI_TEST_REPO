// Include custom Header files
#include "dummy_sens.h"
#include "dummy_leds.h"
#include "read_battery_voltage.h"
#include "nvs_store_data.h"
#include "ble_main.h"
#include "rtc_main.h"


// Include ESP32 Library header files
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
// #include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_pm.h"

// ESP LOG TAG
const char *TAG = "main_app";

// Main app (Code starts from here)
void app_main(void)
{
    vTaskDelay(1000 / (portTICK_PERIOD_MS));
 
    // Set ESP LOG at highest priority
    esp_log_level_set("*", ESP_LOG_ERROR);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    leds_app_start();

    batteryInit(); // Initialize and start the battery voltage (ADC) monitoring task

    // millisInit(); // Initialize and start the millis task

    rtcInit(); // Initialize and start the RTC task

    spiffsInit(); // Initialize and start the SPIFFS for data logging

    sensorOneInit();

    sensorTwoInit();

    bleInit(); // Initialize and start the BLE Service and Callbacks

    // vTaskDelay(1000 / (portTICK_PERIOD_MS));
}