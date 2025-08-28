#include "dummy_sens.h"
#include "ble_main.h"
#include "nvs_store_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_random.h"

float pm10, pm25, pm1, temp1;
int dcp, omr, obst;

float temp2, pres, hum, aqi, co2, voc, mtof;

static char buffer1[256] = {0};  // For sensor one
static char buffer2[600] = {0};  // For sensor two

uint16_t duty_cycling_period = 15;

int led_en2, log_en2;

int duty_val2;

// {CP:1,30,1}

bool is_duty_set = false;

char local_rec_value5_2[32] = {0};

void sens_one_task(void *pvParameters){
    while (1)
    {
        pm10 = 10 + (uint8_t)(esp_random() % 21);
        pm25 = 20 + (uint8_t)(esp_random() % 21);
        pm1 = 30 + (uint8_t)(esp_random() % 21);
        temp1 = 20 + (uint8_t)(esp_random() % 21);
        dcp = 15;
        omr = (uint8_t)(esp_random() % 2);
        obst = (uint8_t)(esp_random() % 2);

        // Buffer to store all the data from the sensor
        snprintf(buffer1, sizeof(buffer1), "{PM10:%.0f,PM25:%.0f,PM1:%.0f,obst:%d,omr:%d,T:%.1f,dcp:%d}\n",
            pm10, pm25, pm1, obst, omr, temp1, dcp); 

        // printf("\nReading actual sensor_data_one\n");
        // printf("%s", buffer1);

        // printf("\nSaving sensor_data_one to nvs...\n");

        // Sends the buffer to the nvs_store_data file
        nvs_read_message_one((const char *)buffer1);

        if (rec_value5 != NULL) {
            strncpy(local_rec_value5_2, rec_value5, sizeof(local_rec_value5_2) - 1);
            local_rec_value5_2[sizeof(local_rec_value5_2) - 1] = '\0';
            sscanf(local_rec_value5_2, "{CP:%d,%d,%d}\n",  &led_en2, &duty_val2, &log_en2);
            duty_cycling_period = duty_val2;

            is_duty_set = true;
        }

        vTaskDelay(((duty_cycling_period*1000) - 100) / portTICK_PERIOD_MS);
    }
}

void sens_two_task(void *pvParameters){
    while (1)
    {
        temp2 = 20 + (uint8_t)(esp_random() % 21);  // Changed to temp2
        pres = 100000.5;
        hum = 60 + (uint8_t)(esp_random() % 21);
        aqi = 50 + (uint8_t)(esp_random() % 21);    // Added aqi initialization
        co2 = 500 + (uint8_t)(esp_random() % 21);
        voc = co2 / 1000;
        mtof = 9.6;

        snprintf(buffer2, sizeof(buffer2), "{T:%.2f,P:%.2f,H:%.2f,IAQ:%.2f,CO2:%.2f,VOC:%.2f,mtof:%.2f}\n", 
            temp2, pres, hum, aqi, co2, voc, mtof);

        // printf("\nReading actual sensor_data_two\n");
        // printf("%s", buffer2);

        // printf("\nSaving sensor_data_two to nvs...\n");

        nvs_read_message_two((const char *)buffer2);

        // printf("sensor_data_two to saved!\n");

        // printf("Reading nvs sensor_data_two\n");

        // check_new_message_two();

        combine_sensor_data();

        vTaskDelay((duty_cycling_period*1000) / portTICK_PERIOD_MS);
    }
}

void sensorOneInit(void) {
    xTaskCreate(sens_one_task, "sens_one_task", 4096, NULL, 5, NULL);
}

void sensorTwoInit(void) {
    xTaskCreate(sens_two_task, "sens_two_task", 4096, NULL, 5, NULL);
}