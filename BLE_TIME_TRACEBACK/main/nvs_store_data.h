#ifndef MAIN_NVS_STORE_H_
#define MAIN_NVS_STORE_H_

#include <esp_system.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <nvs_flash.h>
#include <nvs.h>

#define NVS_TAG "EEPROM"

#define CLEAR_SPIFFS 0

// extern float pm25_val;
extern float led_pm_val;

void spiffsInit(void);
void spiffsCreateFile(void);
void spiffsWrite(const char *data);
void spiffsRead(void);
int spiffsPeekBatch(int count, char *outBuffer, size_t bufSize);
void spiffs_adjust_timestamps(uint32_t ble_timestamp, uint32_t current_seconds_counter);
int spiffsCountPackets(void);
int spiffsDeleteBatch(int count);
void spiffsClearAll(void);

void nvs_read_message_one(const char *nvs_message_one);
void check_new_message_one(void);

void nvs_read_message_two(const char *nvs_message_two);
void check_new_message_two(void);

void nvs_read_message_three(int nvs_message_three);
void check_new_message_three(void);

void combine_sensor_data(void);

// void print_stored_data(void);

#endif /* MAIN_NVS_STORE_H_ */