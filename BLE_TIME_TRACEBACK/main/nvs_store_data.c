// Include custom Header files
#include "nvs_store_data.h"
#include "ble_main.h"
#include "rtc_main.h"

// Include ESP32 Library header files
#include <esp_system.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <nvs_flash.h>
#include <nvs.h>

#include <stdio.h>
#include <string.h>

#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_spiffs.h"

// Variables that store the sensor values from nvs_read_message_one, nvs_read_message_two and nvs_read_message_three
const char *new_message_one;
const char *new_message_two;
int new_message_three;

// Read sensor 1 data and save the value in these variables
float pm10_val, pm25_val, pm1_val, temp1_val;
int dcp_val, obst_val, omr_val;

// Read sensor 2 data and save the value in these variables
float temp2_val, pres_val, hum_val, iaq_val, co2_val, voc_val, mtof_val;

// Value to be sent to the led_main.c to give colour feedback based on current PM value
float led_pm_val = 0.0f;

// Read the battery voltage and store it in this variable
int battery_voltage = 0;

// static char* sensor_buffer[175];

// SPIFFS configuration
esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
};

// File object for create, read, write, modify and delete
FILE* f;

// Buffer to store one complete data packet (Not ll the data to be sent over BLE as 2 packets in one transmit)
static char sensor_buffer[210];

// Initialize SPIFFS, check the partition size and check how much storage is used
void spiffsInit(void){
    // ESP_LOGE(NVS_TAG, "Initializing SPIFFS");
    // printf("Initializing SPIFFS\n");

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            // ESP_LOGE(NVS_TAG, "Failed to mount or format filesystem");
            printf("Failed to mount or format filesystem\n");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            // ESP_LOGE(NVS_TAG, "Failed to find SPIFFS partition");
            printf("Failed to find SPIFFS partition\n");
        } else {
            // ESP_LOGE(NVS_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            printf("Failed to initialize SPIFFS\n");
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        // ESP_LOGE(NVS_TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        printf("Failed to get SPIFFS partition information\n");
        // esp_spiffs_format(conf.partition_label);
        return;
    } else {
        // ESP_LOGI(NVS_TAG, "Partition size: total: %d, used: %d", total, used);
        printf("Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info.
    if (used > total) {
        // ESP_LOGW(NVS_TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        printf("Number of used bytes cannot be larger than total. Performing SPIFFS_check().\n");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            // ESP_LOGE(NVS_TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            printf("SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            // ESP_LOGI(NVS_TAG, "SPIFFS_check() successful");
            printf("SPIFFS_check() successful\n");
        }
    }
}

// Create a new file in the SPIFFS (.txt file is created for now)
void spiffsCreateFile(void){
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    printf("\nOpening file\n");
    f = fopen("/spiffs/hello.txt", "w");
    if (f == NULL) {
        printf("Failed to open file hello.txt for writing\n");
        return;
    }
    fprintf(f, "sensor_data\n");
    fclose(f);
    printf("File written\n");
}

// Write and Append to the .txt file in the SPIFFS
void spiffsWrite(const char *data)
{
    // // Open file in append mode so we add data instead of overwriting
    // f = fopen("/spiffs/hello.txt", "a");

    // fseek(f, 0, SEEK_END);
    // long size = ftell(f);
    // if (size > 0) {
    //     fprintf(f, ", ");
    // }

    // if (f == NULL) {
    //     printf("Failed to open file for appending\n");
    //     return;
    // }
    // fprintf(f, "%s\n", data);  // Add newline after each entry
    // fclose(f);

    // printf("Data appended: %s\n", data);
    
    // Open file in append mode so we add data instead of overwriting
    f = fopen("/spiffs/hello.txt", "a");
    if (f == NULL) {
        printf("Failed to open file for appending\n");
        return;
    }
    fprintf(f, "%s\n", data); // only the JSON object
    fclose(f);
    // printf("Data appended: %s\n", data);
}

// Read the .txt file in the SPIFFS
void spiffsRead(void)
{
    f = fopen("/spiffs/hello.txt", "r");
    if (f == NULL) {
        printf("Failed to open file for reading\n");
        return;
    }

    char line[128];
    printf("File contents:\n");
    while (fgets(line, sizeof(line), f)) {
        // Strip newline for clean printing
        char *pos = strchr(line, '\n');
        if (pos) *pos = '\0';
        printf("%s\n", line);
    }
    fclose(f);
}

// int spiffsPeekBatch(int count, char *outBuffer, size_t bufSize) {
//     FILE *f = fopen("/spiffs/hello.txt", "r");
//     if (!f) {
//         printf("Failed to open file for reading\n");
//         return 0;
//     }

//     char line[512];
//     int readCount = 0;
//     int first = 1;
//     outBuffer[0] = '\0';

//     while (fgets(line, sizeof(line), f) && readCount < count) {
//         // Strip newline
//         char *pos;
//         if ((pos = strchr(line, '\n'))) *pos = '\0';
//         if ((pos = strchr(line, '\r'))) *pos = '\0';

//         if (strlen(line) == 0) continue; // skip empty lines

//         if (!first) strncat(outBuffer, ", ", bufSize - strlen(outBuffer) - 1);
//         strncat(outBuffer, line, bufSize - strlen(outBuffer) - 1);
//         first = 0;
//         readCount++;
//     }
//     fclose(f);

//     return readCount; // number of entries peeked
// }

// Peek 2 data packets from the .txt file from the SPIFFS and put them in the format [{},{}] for transmission
// Returns the number of entries peeked
int spiffsPeekBatch(int count, char *outBuffer, size_t bufSize) {
    FILE *f = fopen("/spiffs/hello.txt", "r");
    if (!f) {
        printf("Failed to open file for reading\n");
        return 0;
    }

    char line[512];
    int readCount = 0;
    int first = 1;
    size_t len = 0;

    // Start JSON array
    strlcpy(outBuffer, "[", bufSize);
    len = strlen(outBuffer);

    while (fgets(line, sizeof(line), f) && readCount < count) {
        // Strip newline/carriage return
        char *pos;
        if ((pos = strchr(line, '\n'))) *pos = '\0';
        if ((pos = strchr(line, '\r'))) *pos = '\0';

        // if (strlen(line) == 0) continue; // skip empty lines

        // Trim whitespace
        char *start = line;
        while (*start == ' ' || *start == '\t' || *start == ',') start++;
        if (*start == '\0') continue; // skip empty or comma-only lines

        if (!first) {
            strlcat(outBuffer, ", ", bufSize); // Add comma between objects
        }
        strlcat(outBuffer, line, bufSize);

        first = 0;
        readCount++;
    }

    // Close JSON array
    strlcat(outBuffer, "]", bufSize);

    fclose(f);

    return readCount; // number of entries peeked
}

// Counts the number of packets in the .txt file from the SPIFFS
int spiffsCountPackets(void)
{
    FILE *f = fopen("/spiffs/hello.txt", "r");
    if (!f) {
        printf("Failed to open file for reading\n");
        return 0;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        // Strip newline/carriage return
        char *pos;
        if ((pos = strchr(line, '\n'))) *pos = '\0';
        if ((pos = strchr(line, '\r'))) *pos = '\0';

        // Skip empty lines
        if (strlen(line) == 0) continue;

        count++;
    }

    fclose(f);

    // printf("Total packets stored: %d\n", count);
    return count;
}

// Deletes the batch (2 packets) from the .txt file from the SPIFFS
// Returns the number of lines skipped
int spiffsDeleteBatch(int count) {
    FILE *src = fopen("/spiffs/hello.txt", "r");
    if (!src) {
        ESP_LOGE("SPIFFS", "Failed to open file for reading");
        return 0;
    }

    FILE *tmp = fopen("/spiffs/tmp.txt", "w");
    if (!tmp) {
        ESP_LOGE("SPIFFS", "Failed to open temp file for writing");
        fclose(src);
        return 0;
    }

    char line[512];
    int skipped = 0;

    while (fgets(line, sizeof(line), src)) {
        if (skipped < count) {
            skipped++;  // just skip these lines
            continue;
        }
        fputs(line, tmp);
    }

    fclose(src);
    fclose(tmp);

    // Remove old file and rename temp
    remove("/spiffs/hello.txt");
    rename("/spiffs/tmp.txt", "/spiffs/hello.txt");

    return skipped; // number of lines deleted
}

// Clear all the data in the .txt file from the SPIFFS
void spiffsClearAll(void)
{
    FILE *f = fopen("/spiffs/hello.txt", "w");
    if (f == NULL) {
        printf("Failed to open file for clearing\n");
        return;
    }
    fclose(f);
    printf("All data cleared from SPIFFS\n");
}

// Read the BMV080 (sensor 1) data buffer 
void nvs_read_message_one(const char *nvs_message_one)
{
    new_message_one = (const char *)nvs_message_one;
}

// Print the BMV080 (sensor 1) data buffer (Used only for debugging)
void check_new_message_one(void){
    printf((const char *)new_message_one);
}

// Read the BME690 (sensor 2) data buffer 
void nvs_read_message_two(const char *nvs_message_two)
{
    new_message_two = (const char *)nvs_message_two;
}

// Print the BME690 (sensor 2) data buffer (Used only for debugging)
void check_new_message_two(void){
    printf((const char *)new_message_two);
}

// Read the Battery voltage data
void nvs_read_message_three(int nvs_message_three)
{
    new_message_three = nvs_message_three;
}

// Print the Battery voltage data (Used only for debugging)
void check_new_message_three(void){
    printf("%d", new_message_three);
}

// Reads the values from the sensors and batter and assigns the values to new variables
// Combines all of the data in a single data packet in the sensor buffer and writes it to the SPIFFS 
void combine_sensor_data(void){

    sscanf(new_message_one, "{PM10:%f,PM25:%f,PM1:%f,obst:%d,omr:%d,T:%f,dcp:%d}\n",  &pm10_val, &pm25_val, &pm1_val, &obst_val, &omr_val, &temp1_val, &dcp_val);
    sscanf(new_message_two, "{T:%f,P:%f,H:%f,IAQ:%f,CO2:%f,VOC:%f,mtof:%f}\n",  &temp2_val, &pres_val, &hum_val, &iaq_val, &co2_val, &voc_val, &mtof_val);

    battery_voltage = new_message_three;

    printf("\n{\"PM10\":\"%.0f\",\"PM25\":\"%.0f\",\"PM1\":\"%.0f\",\"obst\":\"%d\",\"omr\":\"%d\",\"T1\":\"%.02f\",\"dcp\":\"%d\",\"T2\":\"%.02f\",\"P\":\"%.02f\",\"H\":\"%.0f\",\"IAQ\":\"%.0f\",\"CO2\":\"%.0f\",\"VOC\":\"%.02f\",\"mtof\":\"%.02f\",\"Batt\":\"%d\",\"TS\":\"%s\"}\n",
    pm10_val, pm25_val, pm1_val, obst_val, omr_val, temp1_val, dcp_val, temp2_val, pres_val, hum_val, iaq_val, co2_val, voc_val, mtof_val, battery_voltage, current_timestamp_str);

    snprintf((char * __restrict__)sensor_buffer, 210, "{\"PM10\":\"%.0f\",\"PM25\":\"%.0f\",\"PM1\":\"%.0f\",\"obst\":\"%d\",\"omr\":\"%d\",\"T1\":\"%.02f\",\"dcp\":\"%d\",\"T2\":\"%.02f\",\"P\":\"%.02f\",\"H\":\"%.0f\",\"IAQ\":\"%.0f\",\"CO2\":\"%.0f\",\"VOC\":\"%.02f\",\"mtof\":\"%.02f\",\"Batt\":\"%d\",\"TS\":\"%s\"}\n",
    pm10_val, pm25_val, pm1_val, obst_val, omr_val, temp1_val, dcp_val, temp2_val, pres_val, hum_val, iaq_val, co2_val, voc_val, mtof_val, battery_voltage, current_timestamp_str);

    led_pm_val = pm25_val;

    // printf("Writing SPIFFS\n");

    spiffsWrite(sensor_buffer);
}
