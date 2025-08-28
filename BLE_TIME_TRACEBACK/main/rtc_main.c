// Include custom Header files
#include "ESP32Time.h"
#include "nvs_store_data.h"
#include "ble_main.h"

// Include ESP32 Library header files
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "esp_timer.h"

// Declare RTC instance globally so it can be accessed by process_timestamp
static ESP32Time* rtc = NULL;

// Flag to track if RTC has been set from BLE data
static bool rtc_set_from_ble = false;

// String to store current timestamp
char current_timestamp_str[20] = "0"; // Enough for 64-bit timestamp

// Variables for counting seconds from 0 until BLE timestamp is received
uint32_t seconds_counter = 0;
static TickType_t last_tick_count = 0;

void update_timestamp_string() {
    if (rtc_set_from_ble) {
        // Use actual epoch time from RTC
        if (rtc != NULL) {
            time_t epoch = ESP32Time_getEpoch(rtc);
            snprintf(current_timestamp_str, sizeof(current_timestamp_str), "%lld", (long long)epoch);
        }
    } else {
        // Use seconds counter (time since start)
        snprintf(current_timestamp_str, sizeof(current_timestamp_str), "%" PRIu32, seconds_counter);
    }
}

void process_timestamp() {
    // Always check if data is available
    if (rec_value4 == NULL || rec_value4_length == 0) {
        // printf("No BLE data received yet\n");
        return;
    }
    
    // If RTC was already set from BLE, don't process again
    if (rtc_set_from_ble) {
        // printf("RTC already set from BLE, skipping processing\n");
        return;
    }
    
    // printf("Processing %zu bytes of data\n", rec_value4_length);
    
    time_t timestamp = 0;
    int timestamp_valid = 0;
    
    // Try to interpret as Unix timestamp (common formats)
    if (rec_value4_length == 4) {
        // 32-bit integer (most common Unix timestamp format)
        uint32_t timestamp_32;
        memcpy(&timestamp_32, rec_value4, 4);
        timestamp = (time_t)timestamp_32;
        timestamp_valid = 1;
        // printf("32-bit Unix timestamp: %" PRIu32 "\n", timestamp_32);
        
    } else if (rec_value4_length >= 10 && rec_value4_length <= 15) {
        // Possibly a string representation of timestamp
        // Try to convert to integer
        timestamp = (time_t)atol(rec_value4);
        if (timestamp != 0 || rec_value4[0] == '0') {
            timestamp_valid = 1;
            // printf("String Unix timestamp: %lld\n", (long long)timestamp);
        } else {
            // printf("Data as string: %s\n", rec_value4);
        }
        
    } else {
        // Raw data dump
        // printf("Raw data: ");
        for (size_t i = 0; i < rec_value4_length; i++) {
            // printf("%02X ", (unsigned char)rec_value4[i]);
        }
        // printf("\n");
    }
    
    // Set RTC if we have a valid timestamp and it hasn't been set yet
    if (timestamp_valid && rtc != NULL && !rtc_set_from_ble) {
        // printf("Setting RTC to epoch: %lld\n", (long long)timestamp);
        ESP32Time_setTimeEpoch(rtc, timestamp, 0);
        
        // Verify the new time was set correctly
        // time_t current_epoch = ESP32Time_getEpoch(rtc);
        // printf("RTC now set to: %lld\n", (long long)current_epoch);
        // printf("Human readable: %s", ctime(&current_epoch));
        
        // Set the flag to indicate RTC is now set from BLE
        rtc_set_from_ble = true;

        spiffs_adjust_timestamps(timestamp, seconds_counter);
        
        // Update the timestamp string to use epoch time
        update_timestamp_string();
        // printf("Timestamp stored as string: %s\n", current_timestamp_str);
        
        // Optional: Free the BLE data since we don't need it anymore
        free(rec_value4);
        rec_value4 = NULL;
        rec_value4_length = 0;
    }
}

void rtc_task(void *pvParameters) {
    // Create RTC instance and store globally
    rtc = ESP32Time_create();
    if (rtc == NULL) {
        // printf("Failed to create RTC instance!\n");
        vTaskDelete(NULL);
        return;
    }
    
    // Set initial time (this won't be used until BLE timestamp is received)
    ESP32Time_setTimeEpoch(rtc, 1755872195, 0);
    // printf("Initial RTC set to epoch: %lu\n", ESP32Time_getEpoch(rtc));
    
    // Initialize timestamp string with 0
    update_timestamp_string();
    
    // Initialize tick count for seconds counter
    last_tick_count = xTaskGetTickCount();
    
    // Pre-allocate buffers to avoid repeated malloc/free
    char time_buffer[64];
    char date_buffer[64];
    
    for(;;) {
        // Update seconds counter if BLE timestamp not received yet
        if (!rtc_set_from_ble) {
            TickType_t current_tick = xTaskGetTickCount();
            TickType_t elapsed_ticks = current_tick - last_tick_count;
            
            // Count seconds (1000ms = 1 second)
            if (elapsed_ticks >= pdMS_TO_TICKS(1000)) {
                seconds_counter++;
                last_tick_count = current_tick;
                update_timestamp_string();
            }
        } else {
            // Get time strings using a safer approach
            struct tm timeinfo = ESP32Time_getTimeStruct(rtc);
            
            // Format time and date directly into buffers
            strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &timeinfo);
            strftime(date_buffer, sizeof(date_buffer), "%a, %b %d %Y", &timeinfo);
            
            // printf("Time: %s\n", time_buffer);
            // printf("Date: %s\n", date_buffer);
            // printf("Epoch: %lu\n", ESP32Time_getEpoch(rtc));
        }
        
        // Update timestamp string (handles both cases)
        update_timestamp_string();
        // printf("Current timestamp string: %s\n", current_timestamp_str);

        // Process any incoming BLE timestamp data
        process_timestamp();
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Clean up (this line will never be reached in infinite loop)
    ESP32Time_destroy(rtc);
    rtc = NULL;
    vTaskDelete(NULL);
}

void get_millis_task(void *pvParameters){
    while(1) {
        int64_t time_us = esp_timer_get_time();
        int64_t time_ms = time_us / 1000;

        printf("time_ms: %lld\n", time_ms);

        // Delay for 1000ms (1 second) before the next iteration of the loop
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void millisInit(void) {
    xTaskCreate(get_millis_task, "get_millis_task", 1024, NULL, 5, NULL);
}

void rtcInit(void) {
    // Increased stack size to 4096 bytes
    xTaskCreate(rtc_task, "rtc_task", 4096, NULL, 5, NULL);
}