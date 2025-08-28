// Include custom Header files
#include "read_battery_voltage.h"
#include "nvs_store_data.h"

// Include ESP32 Library header files
#include <stdio.h>

#include "freertos/FreeRTOS.h"

#include "driver/adc.h"

// ADC value variable stores the data from ADC1_CH0 ~ Pin GPIO1
int adc_value = 0;

// Task to read the ADC1_CH0 and send the value to the nvs_store_data file
void readAdc(void *pvParameters){
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
    adc1_config_width(ADC_WIDTH_BIT_12);

    while(true)
    {
        adc_value = adc1_get_raw(ADC1_CHANNEL_0); 
        // printf("ADC Value is %d\n", adc_value);

        nvs_read_message_three(adc_value);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void batteryInit(void){
    xTaskCreate(readAdc, "readAdc", 1024, NULL, 5, NULL);
}