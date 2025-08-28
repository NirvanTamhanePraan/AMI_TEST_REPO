// Include custom Header files
#include "ble_main.h"
#include "nvs_store_data.h"
#include "sdkconfig.h"

// Include ESP32 Library header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

// ESP LOG TAG
// static const char *GATTS_TAG = "CLIP-SENSOR";

// Define BLE Profile, App ID, and Number of Handles
#define PROFILE_NUM 1
#define PROFILE_APP_ID 0
#define NUM_HANDLE 15 // (1x Service, 3x Characteristics, 3x Descriptors, and their values)

// UUIDs for the Sevice and 3 Characteristics
// GATTS_SERVICE_UUID: "00000001-0000-1000-8000-00805F9B34FB"
// GATTS_CHAR_UUID: "00002B00-0000-1000-8000-00805F9B34FB"
// GATTS_CHAR2_UUID: "00002B02-0000-1000-8000-00805F9B34FB"
// GATTS_DESCR_UUID: "00002902-0000-1000-8000-00805F9B34FB"
// GATTS_CHAR3_UUID: "00002B03-0000-1000-8000-00805F9B34FB"
// GATTS_DESCR_UUID: "00002902-0000-1000-8000-00805F9B34FB"
// GATTS_CHAR4_UUID: "00002B04-0000-1000-8000-00805F9B34FB"
// GATTS_DESCR_UUID: "00002902-0000-1000-8000-00805F9B34FB"

// 128 bit UUIDs in little endian format for the Sevice and 3 Characteristics
static const uint8_t GATTS_SERVICE_UUID128[16] = {
    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

static const uint8_t GATTS_CHAR_UUID128[16] = {
    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00
};

static const uint8_t GATTS_CHAR2_UUID128[16] = {
    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x02, 0x2B, 0x00, 0x00
};

static const uint8_t GATTS_CHAR3_UUID128[16] = {
    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x03, 0x2B, 0x00, 0x00
};

static const uint8_t GATTS_CHAR4_UUID128[16] = {
    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x04, 0x2B, 0x00, 0x00
};

// Macro for changing the name of the ble sensor device
static char test_device_name[ESP_BLE_ADV_NAME_LEN_MAX] = "PRAAN_AMIE_";

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

// 128 bit UUID in litte endian format for advertising the service UUID
static uint8_t adv_service_uuid128[16] = {
    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

// Structure for gatts_profile_inst
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    uint16_t char2_handle;
    esp_bt_uuid_t char2_uuid;
    uint16_t char3_handle;
    esp_bt_uuid_t char3_uuid;
    uint16_t char4_handle;
    esp_bt_uuid_t char4_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
    uint16_t descr2_handle;
    esp_bt_uuid_t descr2_uuid;
    uint16_t descr3_handle;
    esp_bt_uuid_t descr3_uuid;
    uint16_t descr4_handle;
    esp_bt_uuid_t descr4_uuid;
};

// Declaring the functions
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void example_write_event_env(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// Characteristic property [Set later in the code (N/R/W/WNR)]
static esp_gatt_char_prop_t char_property = 0;

char macStr[18]= {0};

// Variable arrays to store the characteristic values
static uint8_t char_value_read1[512] = {0};
static uint8_t char_value_read2[1] = {0};
static char char_value_read3[5] = {0};
static uint8_t char_value_read4[1] = {0};

// Variable array to store the current batch value
char batch[512];

// Variable array to store the current unix timestamp value
char unix_time[5];

char *rec_value4 = NULL;
size_t rec_value4_length = 0;

// Variable array to store the previous batch value
static char last_sent_batch[512] = {0};

// Variable array to store the blank packet value
// static char blank_array[512] = "[{}]";

// Variable to store the number of stored data packets
int count_stored_packets = 0;

// Variable to store the number of available data packets (Used for charachteristic 3)
int available_packets = 0;

// Flag to indicate the creation of the BLE Service
static bool service_create_cmpl = false;

// Flags to indicate notifications have been enabled via App
static bool notify_enabled_char1 = false;
static bool notify_enabled_char2 = false;
static bool notify_enabled_char3 = false;
static bool notify_enabled_char4 = false;

// Current MTU size for the active connection.
static uint16_t local_mtu = 512;

// Used for debugging
// const char *sensor_message;
// char saved_message[512];

// Service
static uint8_t adv_config_done = 0;

// Characteristic 1 Attribute value struct
static esp_attr_value_t char_value_send_read1 = {
    .attr_max_len = 512,
    .attr_len = sizeof(char_value_read1),
    .attr_value = char_value_read1,
};

// Characteristic 2 Attribute value struct
static esp_attr_value_t char_value_send_read2 =
{
    .attr_max_len = 1,
    .attr_len = sizeof(char_value_read2),
    .attr_value = char_value_read2,
};

// Characteristic 3 Attribute value struct
static esp_attr_value_t char_value_send_read3 =
{
    .attr_max_len = sizeof(char_value_read3),  // Now 5 bytes
    .attr_len = 0,  // Will be set dynamically (0 to 9999)
    .attr_value = (uint8_t *)char_value_read3,
};

// Characteristic 4 Attribute value struct
static esp_attr_value_t char_value_send_read4 =
{
    .attr_max_len = 1,
    .attr_len = sizeof(char_value_read4),
    .attr_value = char_value_read4,
};

// Advertisement data struct (Includes the Service UUID to be advertised)
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// Advertisement parameter data struct
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20, // 20ms
    .adv_int_max = 0x40, // 40ms
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// GATT Profile struct
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_ID] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

void getDeviceAddress(void){
    const uint8_t *point = esp_bt_dev_get_address();
    if (point != NULL) {       
        snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X", point[0], point[1], point[2], point[3], point[4], point[5]);
        strcat(test_device_name, macStr);
    }
}

// Read the number of packets stored, set the attribute value and notify the client
void readCountPackets(void){
    count_stored_packets = spiffsCountPackets();

    // Convert integer to string - using snprintf for safety
    int written = snprintf(char_value_read3, sizeof(char_value_read3), "%d", count_stored_packets);
    if (written >= sizeof(char_value_read3)) {
        // Handle truncation if needed (unlikely with 5 byte buffer)
        char_value_read3[sizeof(char_value_read3)-1] = '\0';
        written = sizeof(char_value_read3)-1;
    }

    // Update the attribute length (don't include null terminator)
    char_value_send_read3.attr_len = written;

    // printf("Sending stored packet count over BLE: %s\n", char_value_read3);

    // Update the attribute value
    esp_ble_gatts_set_attr_value(gl_profile_tab[PROFILE_APP_ID].char3_handle, 
                                char_value_send_read3.attr_len, 
                                (uint8_t *)char_value_read3);

    // Send notification if enabled
    if (notify_enabled_char3) {
        esp_ble_gatts_send_indicate(
            gl_profile_tab[PROFILE_APP_ID].gatts_if,
            gl_profile_tab[PROFILE_APP_ID].conn_id,
            gl_profile_tab[PROFILE_APP_ID].char3_handle,
            char_value_send_read3.attr_len,
            (uint8_t *)char_value_read3,
            false
        );
    }
}

// Task to read the packets, set the attribute values to the characteristics and notify the client
static void read_set_sensor_data_task(void* param) {
    while (1) {
        if (service_create_cmpl) {   
            
            // readCountPackets();

            available_packets = spiffsCountPackets();
            // printf("Packets available: %d\n", available_packets);

            if(available_packets >= 2){
                if (spiffsPeekBatch(2, batch, sizeof(batch))) {
                    // printf("Peeked %d packets: %s\n", 2, batch);
                    // Only proceed if data has changed
                    if (strcmp(batch, last_sent_batch) != 0) {
                        // printf("New data detected, sending over BLE: %s\n", batch);

                        // Clear and copy the data packet
                        memset(char_value_read1, 0, sizeof(char_value_read1));
                        size_t data_len = strlen(batch);
                        if (data_len >= sizeof(char_value_read1)) {
                            data_len = sizeof(char_value_read1) - 1;
                        }
                        memcpy(char_value_read1, batch, data_len);
                        
                        // Update the actual length
                        char_value_send_read1.attr_len = data_len;
                        
                        // Update attribute value with correct length
                        esp_ble_gatts_set_attr_value(
                            gl_profile_tab[PROFILE_APP_ID].char_handle,
                            data_len,
                            char_value_read1
                        );

                        // // Send notification if enabled
                        // if (notify_enabled_char1) {
                        //     esp_ble_gatts_send_indicate(
                        //         gl_profile_tab[PROFILE_APP_ID].gatts_if,
                        //         gl_profile_tab[PROFILE_APP_ID].conn_id,
                        //         gl_profile_tab[PROFILE_APP_ID].char_handle,
                        //         strlen(batch),
                        //         char_value_read1,
                        //         false
                        //     );
                        // }

                        char_value_read2[0] = 0x31;

                        // printf("Sending confirmation flag over BLE: %u\n", char_value_read2[0]);

                        // Update the attribute value
                        esp_ble_gatts_set_attr_value(gl_profile_tab[PROFILE_APP_ID].char2_handle, 
                                                    sizeof(char_value_read2), 
                                                    char_value_read2);

                        // Send notification if enabled
                        if (notify_enabled_char2) {
                            esp_ble_gatts_send_indicate(
                                gl_profile_tab[PROFILE_APP_ID].gatts_if, 
                                gl_profile_tab[PROFILE_APP_ID].conn_id,
                                gl_profile_tab[PROFILE_APP_ID].char2_handle,
                                sizeof(char_value_read2),
                                char_value_read2,
                                false
                            );
                        }
                    
                        // Store the current batch as last sent
                        strncpy(last_sent_batch, batch, sizeof(last_sent_batch) - 1);
                        last_sent_batch[sizeof(last_sent_batch) - 1] = '\0'; // Null termination
                    }
                    // else {
                    //     // printf("Data unchanged, skipping BLE send\n");
                    //     memcpy(char_value_read1, blank_array, strlen(blank_array));
                        
                    //     // Update the actual length
                    //     char_value_send_read1.attr_len = strlen(blank_array);
                        
                    //     // Update attribute value with correct length
                    //     esp_ble_gatts_set_attr_value(
                    //         gl_profile_tab[PROFILE_APP_ID].char_handle,
                    //         strlen(blank_array),
                    //         char_value_read1
                    //     );
                        
                    //     // Send notification if enabled
                    //     if (notify_enabled_char1) {
                    //         uint16_t notify_len = (strlen(blank_array) > local_mtu - 3) ? 
                    //                             (local_mtu - 3) : strlen(blank_array);
                    //         esp_ble_gatts_send_indicate(
                    //             gl_profile_tab[PROFILE_APP_ID].gatts_if,
                    //             gl_profile_tab[PROFILE_APP_ID].conn_id,
                    //             gl_profile_tab[PROFILE_APP_ID].char_handle,
                    //             notify_len,
                    //             char_value_read1,
                    //             false
                    //         );
                    //     }
                    // }
                }
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// GAP Evet handler (Server creation and advertisement)
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            // ESP_LOGE(GATTS_TAG, "Advertising data set, status %d", param->adv_data_cmpl.status);
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            // ESP_LOGE(GATTS_TAG, "Scan response data set, status %d", param->scan_rsp_data_cmpl.status);
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                // ESP_LOGE(GATTS_TAG, "Advertising start failed, status %d", param->adv_start_cmpl.status);
                break;
            }
            // ESP_LOGE(GATTS_TAG, "Advertising start successfully");
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            // ESP_LOGE(GATTS_TAG, "Connection params update, status %d, conn_int %d, latency %d, timeout %d",
            //         param->update_conn_params.status,
            //         param->update_conn_params.conn_int,
            //         param->update_conn_params.latency,
            //         param->update_conn_params.timeout);
            break;
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
            // ESP_LOGE(GATTS_TAG, "Packet length update, status %d, rx %d, tx %d",
            //         param->pkt_data_length_cmpl.status,
            //         param->pkt_data_length_cmpl.params.rx_len,
            //         param->pkt_data_length_cmpl.params.tx_len);
            break;
        default:
            break;
    }
}

// GATTS Profile Event Handler (Create the Service, Characteristics, Descriptor and Handle all the callbacks)
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            // ESP_LOGI(GATTS_TAG, "GATT server register, status %d, app_id %d", param->reg.status, param->reg.app_id);
            gl_profile_tab[PROFILE_APP_ID].service_id.is_primary = true;
            gl_profile_tab[PROFILE_APP_ID].service_id.id.inst_id = 0x00;
            gl_profile_tab[PROFILE_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_128;
            memcpy(gl_profile_tab[PROFILE_APP_ID].service_id.id.uuid.uuid.uuid128, GATTS_SERVICE_UUID128, 16);
            
            getDeviceAddress();

            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(test_device_name);
            if (set_dev_name_ret)
            {
                // ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret) {
                // ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
                break;
            }
            esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_APP_ID].service_id, NUM_HANDLE);
            break;
        case ESP_GATTS_CREATE_EVT:
            //service has been created, now add characteristic declaration
            // ESP_LOGI(GATTS_TAG, "Service create, status %d, service_handle %d", param->create.status, param->create.service_handle);
            gl_profile_tab[PROFILE_APP_ID].service_handle = param->create.service_handle;

            char_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

            gl_profile_tab[PROFILE_APP_ID].char_uuid.len = ESP_UUID_LEN_128;
            memcpy(gl_profile_tab[PROFILE_APP_ID].char_uuid.uuid.uuid128, GATTS_CHAR_UUID128, 16);

            ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                        &gl_profile_tab[PROFILE_APP_ID].char_uuid,
                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                        char_property,
                                        &char_value_send_read1, 
                                        NULL);

            if (ret) {
                // ESP_LOGE(GATTS_TAG, "add char 1 failed, error code = %x", ret);
            }

            esp_ble_gatts_start_service(gl_profile_tab[PROFILE_APP_ID].service_handle);

            break;
        case ESP_GATTS_ADD_CHAR_EVT:
            // ESP_LOGI(GATTS_TAG, "Characteristics add, status %d, attr_handle %d, char_uuid %x", 
            //         param->add_char.status, 
            //         param->add_char.attr_handle, 
            //         param->add_char.char_uuid.uuid.uuid128);

            if (gl_profile_tab[PROFILE_APP_ID].char_handle == 0){
                gl_profile_tab[PROFILE_APP_ID].char_handle = param->add_char.attr_handle;
                gl_profile_tab[PROFILE_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
                gl_profile_tab[PROFILE_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

                // ESP_LOGI(GATTS_TAG, "char 1 handle %d", param->add_char.attr_handle);
                ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                                &gl_profile_tab[PROFILE_APP_ID].descr_uuid,
                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                NULL, 
                                                NULL);
                if (ret) {
                    // ESP_LOGE(GATTS_TAG, "add char 1 failed, error code = %x", ret);
                }  

                gl_profile_tab[PROFILE_APP_ID].char2_uuid.len = ESP_UUID_LEN_128;
                memcpy(gl_profile_tab[PROFILE_APP_ID].char2_uuid.uuid.uuid128, GATTS_CHAR2_UUID128, 16);

                ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                            &gl_profile_tab[PROFILE_APP_ID].char2_uuid,
                                            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                            char_property,
                                            &char_value_send_read2, 
                                            NULL);

                if (ret) {
                    // ESP_LOGE(GATTS_TAG, "add char 2 failed, error code = %x", ret);
                }

            }

            else if (gl_profile_tab[PROFILE_APP_ID].char2_handle == 0){
                gl_profile_tab[PROFILE_APP_ID].char2_handle = param->add_char.attr_handle;
                gl_profile_tab[PROFILE_APP_ID].descr2_uuid.len = ESP_UUID_LEN_16;
                gl_profile_tab[PROFILE_APP_ID].descr2_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

                // ESP_LOGI(GATTS_TAG, "char 2 handle %d", param->add_char.attr_handle);
                ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                                &gl_profile_tab[PROFILE_APP_ID].descr2_uuid,
                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                NULL, 
                                                NULL);

                if (ret) {
                    // ESP_LOGE(GATTS_TAG, "add char 2 failed, error code = %x", ret);
                }

                gl_profile_tab[PROFILE_APP_ID].char3_uuid.len = ESP_UUID_LEN_128;
                memcpy(gl_profile_tab[PROFILE_APP_ID].char3_uuid.uuid.uuid128, GATTS_CHAR3_UUID128, 16);

                ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                            &gl_profile_tab[PROFILE_APP_ID].char3_uuid,
                                            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                            char_property,
                                            &char_value_send_read3, 
                                            NULL);

                if (ret) {
                    // ESP_LOGE(GATTS_TAG, "add char 3 failed, error code = %x", ret);
                }
            }
            
            else if (gl_profile_tab[PROFILE_APP_ID].char3_handle == 0){
                gl_profile_tab[PROFILE_APP_ID].char3_handle = param->add_char.attr_handle;
                gl_profile_tab[PROFILE_APP_ID].descr3_uuid.len = ESP_UUID_LEN_16;
                gl_profile_tab[PROFILE_APP_ID].descr3_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

                // ESP_LOGI(GATTS_TAG, "char 3 handle %d", param->add_char.attr_handle);
                ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                                &gl_profile_tab[PROFILE_APP_ID].descr3_uuid,
                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                NULL, 
                                                NULL);

                if (ret) {
                    // ESP_LOGE(GATTS_TAG, "add char 3 failed, error code = %x", ret);
                }

                gl_profile_tab[PROFILE_APP_ID].char4_uuid.len = ESP_UUID_LEN_128;
                memcpy(gl_profile_tab[PROFILE_APP_ID].char4_uuid.uuid.uuid128, GATTS_CHAR4_UUID128, 16);

                ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                            &gl_profile_tab[PROFILE_APP_ID].char4_uuid,
                                            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                            char_property,
                                            &char_value_send_read4, 
                                            NULL);

                if (ret) {
                    // ESP_LOGE(GATTS_TAG, "add char 3 failed, error code = %x", ret);
                }
            }

            else if (gl_profile_tab[PROFILE_APP_ID].char4_handle == 0){
                gl_profile_tab[PROFILE_APP_ID].char4_handle = param->add_char.attr_handle;
                gl_profile_tab[PROFILE_APP_ID].descr4_uuid.len = ESP_UUID_LEN_16;
                gl_profile_tab[PROFILE_APP_ID].descr4_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

                // ESP_LOGI(GATTS_TAG, "char 4 handle %d", param->add_char.attr_handle);
                ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_APP_ID].service_handle, 
                                                &gl_profile_tab[PROFILE_APP_ID].descr4_uuid,
                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                NULL, 
                                                NULL);

                if (ret) {
                    // ESP_LOGE(GATTS_TAG, "add char 4 failed, error code = %x", ret);
                }
            }

            break;
        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            // ESP_LOGI(GATTS_TAG, "Descriptor add, status %d, attr_handle %u", 
            //         param->add_char_descr.status, 
            //         param->add_char_descr.attr_handle);

            if (gl_profile_tab[PROFILE_APP_ID].descr_handle == 0) {
                gl_profile_tab[PROFILE_APP_ID].descr_handle = param->add_char_descr.attr_handle;
            } 
            else if (gl_profile_tab[PROFILE_APP_ID].descr2_handle == 0){
                gl_profile_tab[PROFILE_APP_ID].descr2_handle = param->add_char_descr.attr_handle;
            }
            else if (gl_profile_tab[PROFILE_APP_ID].descr3_handle == 0){
                gl_profile_tab[PROFILE_APP_ID].descr3_handle = param->add_char_descr.attr_handle;
            }
            else if (gl_profile_tab[PROFILE_APP_ID].descr4_handle == 0){
                gl_profile_tab[PROFILE_APP_ID].descr4_handle = param->add_char_descr.attr_handle;
            }

            service_create_cmpl = true;
            break;
        case ESP_GATTS_READ_EVT:
            // ESP_LOGI(GATTS_TAG, "Characteristic read");
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));

            if (param->read.handle == gl_profile_tab[PROFILE_APP_ID].char_handle) {
                // uint16_t offset = param->read.offset;
                // uint16_t value_len = char_value_send_read1.attr_len;
                
                // uint16_t max_mtu_payload = local_mtu - 1;
                // uint16_t remaining_len = value_len - offset;
                // uint16_t send_len = (remaining_len > max_mtu_payload) ? max_mtu_payload : remaining_len;

                // // if (offset > value_len) {
                // //     ESP_LOGE("BLE", "Offset out of range");
                // //     rsp.attr_value.len = 0;
                // //     esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                // //         param->read.trans_id, ESP_GATT_INVALID_OFFSET, &rsp);
                // //     break;
                // // }

                // rsp.attr_value.handle = param->read.handle;
                // memcpy(rsp.attr_value.value, &char_value_read1[offset], send_len);
                // rsp.attr_value.len = send_len;

                // esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);

                available_packets = spiffsCountPackets();
                // printf("Packets available: %d\n", available_packets);

                if(available_packets >= 2) {
                    // Clear and copy the data packet
                    memset(char_value_read1, 0, sizeof(char_value_read1));
                    size_t data_len = strlen(batch);
                    if (data_len >= sizeof(char_value_read1)) {
                        data_len = sizeof(char_value_read1) - 1;
                    }
                    memcpy(char_value_read1, batch, data_len);
                    
                    // Update the actual length
                    char_value_send_read1.attr_len = data_len;

                    printf("char_value_send_read1.attr_len: %u\n", char_value_send_read1.attr_len);
                    
                    // Update attribute value with correct length
                    esp_ble_gatts_set_attr_value(
                        gl_profile_tab[PROFILE_APP_ID].char_handle,
                        data_len,
                        char_value_read1
                    );

                    uint16_t offset = param->read.offset;
                    uint16_t value_len = char_value_send_read1.attr_len;
                    
                    uint16_t max_mtu_payload = local_mtu - 1;
                    uint16_t remaining_len = value_len - offset;
                    uint16_t send_len = (remaining_len > max_mtu_payload) ? max_mtu_payload : remaining_len;

                    // if (offset > value_len) {
                    //     ESP_LOGE("BLE", "Offset out of range");
                    //     rsp.attr_value.len = 0;
                    //     esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                    //         param->read.trans_id, ESP_GATT_INVALID_OFFSET, &rsp);
                    //     break;
                    // }

                    rsp.attr_value.handle = param->read.handle;
                    memcpy(rsp.attr_value.value, &char_value_read1[offset], send_len);
                    rsp.attr_value.len = send_len;

                    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
                }
            }

            else if (param->read.handle == gl_profile_tab[PROFILE_APP_ID].char2_handle){
                rsp.attr_value.handle = param->read.handle;
                rsp.attr_value.len = sizeof(char_value_read2);
                memcpy(rsp.attr_value.value, char_value_read2, sizeof(char_value_read2));
                // esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            }

            else if (param->read.handle == gl_profile_tab[PROFILE_APP_ID].char3_handle) {
                rsp.attr_value.handle = param->read.handle;
                rsp.attr_value.len = sizeof((uint8_t *)char_value_read3);
                memcpy(rsp.attr_value.value, (uint8_t *)char_value_read3, rsp.attr_value.len);
                // esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            }

            else if (param->read.handle == gl_profile_tab[PROFILE_APP_ID].char4_handle){
                rsp.attr_value.handle = param->read.handle;
                rsp.attr_value.len = sizeof(char_value_read4);
                memcpy(rsp.attr_value.value, char_value_read4, sizeof(char_value_read4));
                // esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            }

            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;
        case ESP_GATTS_WRITE_EVT:
            // ESP_LOGI(GATTS_TAG, "Characteristic write, value len %u, value ", param->write.len);
            // ESP_LOG_BUFFER_HEX(GATTS_TAG, param->write.value, param->write.len);
            
            // Check if this is a write to the Client Characteristic Configuration Descriptor (CCCD)
            if (gl_profile_tab[PROFILE_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                
                if (descr_value == 0x0001) {
                    // Notification enabled for char1
                    if (char_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                        // ESP_LOGI(GATTS_TAG, "Notification enable for char1");
                        notify_enabled_char1 = true;
                    }
                } 
                else if (descr_value == 0x0000) {
                    // Notifications disabled for char1
                    notify_enabled_char1 = false;
                    // ESP_LOGI(GATTS_TAG, "Notification disable for char1");
                } 
                else {
                    // ESP_LOGE(GATTS_TAG, "Invalid descriptor value");
                    // ESP_LOG_BUFFER_HEX(GATTS_TAG, param->write.value, param->write.len);
                }
            } 
            else if (gl_profile_tab[PROFILE_APP_ID].descr2_handle == param->write.handle && param->write.len == 2) {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001) {
                    // Notification enabled for char2
                    // ESP_LOGI(GATTS_TAG, "Notification enable for char2");
                    notify_enabled_char2 = true;
                } else if (descr_value == 0x0000) {
                    // Notifications disabled for char2
                    // ESP_LOGI(GATTS_TAG, "Notification disable for char2");
                    notify_enabled_char2 = false;
                }
            }

            else if (gl_profile_tab[PROFILE_APP_ID].descr3_handle == param->write.handle && param->write.len == 2) {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001) {
                    // Notification enabled for char2
                    // ESP_LOGI(GATTS_TAG, "Notification enable for char2");
                    notify_enabled_char3 = true;
                } else if (descr_value == 0x0000) {
                    // Notifications disabled for char2
                    // ESP_LOGI(GATTS_TAG, "Notification disable for char2");
                    notify_enabled_char3 = false;
                }
            }

            else if (gl_profile_tab[PROFILE_APP_ID].descr4_handle == param->write.handle && param->write.len == 2) {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001) {
                    // Notification enabled for char2
                    // ESP_LOGI(GATTS_TAG, "Notification enable for char2");
                    notify_enabled_char4 = true;
                } else if (descr_value == 0x0000) {
                    // Notifications disabled for char2
                    // ESP_LOGI(GATTS_TAG, "Notification disable for char2");
                    notify_enabled_char4 = false;
                }
            }

            if (!param->write.is_prep)
                {
                    // #if BLE_DEBUG == 1
                    // ESP_LOGE(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
                    // #endif
                    // ESP_LOG_BUFFER_HEX(GATTS_TAG, param->write.value, param->write.len);

                    if (param->write.handle == gl_profile_tab[PROFILE_APP_ID].char_handle) {
                        // First characteristic write
                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "First char write: %.*s", param->write.len, param->write.value);
                        // #endif

                        // char *rec_value = (char *)param->write.value;

                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Received value 1: %s", rec_value);
                        // #endif
                    }
                    else if (param->write.handle == gl_profile_tab[PROFILE_APP_ID].char2_handle) {
                        // Second characteristic write
                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Second char write: %.*s", param->write.len, param->write.value);
                        // #endif

                        char *rec_value2 = (char *)param->write.value;

                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Received value 2: %s", rec_value2);
                        // #endif

                        if (strcmp(rec_value2, "0") == 0) {
                            spiffsDeleteBatch(2);
                        }
                    }
                    else if (param->write.handle == gl_profile_tab[PROFILE_APP_ID].char3_handle) {
                        // Second characteristic write
                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Second char write: %.*s", param->write.len, param->write.value);
                        // #endif

                        char *rec_value3 = (char *)param->write.value;

                        if (strcmp(rec_value3, "0") == 0) {
                            readCountPackets();
                        }

                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Received value 2: %s", rec_value2);
                        // #endif
                    }
                    else if (param->write.handle == gl_profile_tab[PROFILE_APP_ID].char4_handle) {
                        // Second characteristic write
                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Second char write: %.*s", param->write.len, param->write.value);
                        // #endif

                        // char *rec_value4 = (char *)param->write.value;

                        // Free previous allocation if any
                        free(rec_value4);
                        
                        // Use the actual length from the BLE parameter
                        rec_value4_length = param->write.len;
                        
                        // Allocate memory for the data (+1 for optional null termination)
                        rec_value4 = malloc(rec_value4_length + 1);
                        if (rec_value4 == NULL) {
                            // Handle allocation failure
                            rec_value4_length = 0;
                            return;
                        }
                        
                        // Copy the exact number of bytes received
                        memcpy(rec_value4, param->write.value, rec_value4_length);
                        
                        // Optional: Add null terminator if you want to treat it as a string
                        rec_value4[rec_value4_length] = '\0';
                        
                        // Debug: Print what was received
                        printf("Received %zu bytes: ", rec_value4_length);
                        for (size_t i = 0; i < rec_value4_length; i++) {
                            printf("%02X ", (unsigned char)rec_value4[i]);
                        }
                        printf("\n");

                        if (strcmp(rec_value4, "1755841847") == 0) {

                            snprintf(unix_time, sizeof(unix_time), "%s", rec_value4);

                            printf("unix_time = %s\n", unix_time);

                            char_value_read4[0] = 0x31;

                            // printf("Sending confirmation flag over BLE: %u\n", char_value_read4[0]);

                            // Update the attribute value
                            esp_ble_gatts_set_attr_value(gl_profile_tab[PROFILE_APP_ID].char4_handle, 
                                                        sizeof(char_value_read4), 
                                                        char_value_read4);

                            // Send notification if enabled
                            if (notify_enabled_char4) {
                                esp_ble_gatts_send_indicate(
                                    gl_profile_tab[PROFILE_APP_ID].gatts_if, 
                                    gl_profile_tab[PROFILE_APP_ID].conn_id,
                                    gl_profile_tab[PROFILE_APP_ID].char4_handle,
                                    sizeof(char_value_read4),
                                    char_value_read4,
                                    false
                                );
                            }
                        }

                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Received value 2: %s", rec_value2);
                        // #endif
                    }
                    else {
                        // #if BLE_DEBUG == 1
                        // ESP_LOGE(GATTS_TAG, "Write to unknown handle: %d", handle);
                        // #endif
                    }
                    // Send a response for Write Requests
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }

            example_write_event_env(gatts_if, param);
            break;
        case ESP_GATTS_DELETE_EVT:
            break;
        case ESP_GATTS_START_EVT:
            // ESP_LOGI(GATTS_TAG, "Service start, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_STOP_EVT:
            break;
        case ESP_GATTS_CONNECT_EVT:
            // ESP_LOGI(GATTS_TAG, "Connected, conn_id %u, remote "ESP_BD_ADDR_STR"", 
            //         param->connect.conn_id, 
            //         ESP_BD_ADDR_HEX(param->connect.remote_bda));
            gl_profile_tab[PROFILE_APP_ID].conn_id = param->connect.conn_id;

            readCountPackets();

            break;
        case ESP_GATTS_DISCONNECT_EVT:
            // ESP_LOGI(GATTS_TAG, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x", 
            //         ESP_BD_ADDR_HEX(param->disconnect.remote_bda), 
            //         param->disconnect.reason);
            notify_enabled_char1 = false;
            notify_enabled_char2 = false;
            notify_enabled_char3 = false;
            esp_ble_gap_start_advertising(&adv_params);
            // local_mtu = 512;
            break;
        case ESP_GATTS_CONF_EVT:
            // ESP_LOGI(GATTS_TAG, "Confirm receive, status %d, attr_handle %d", param->conf.status, param->conf.handle);
            if (param->conf.status != ESP_GATT_OK) {
                // ESP_LOG_BUFFER_HEX(GATTS_TAG, param->conf.value, param->conf.len);
            }
            break;
        case ESP_GATTS_SET_ATTR_VAL_EVT:
            // ESP_LOGI(GATTS_TAG, "Attribute value set, status %d", param->set_attr_val.status);

            // if (notify_enabled_char1) {                    
            //     esp_ble_gatts_send_indicate(
            //         gl_profile_tab[PROFILE_APP_ID].gatts_if,
            //         gl_profile_tab[PROFILE_APP_ID].conn_id,
            //         gl_profile_tab[PROFILE_APP_ID].char_handle,
            //         strlen(batch),
            //         char_value_read1,
            //         false
            //     );

            // }

            // if (notify_enabled_char2) {
            //     esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_APP_ID].gatts_if, 
            //                             gl_profile_tab[PROFILE_APP_ID].conn_id,
            //                             gl_profile_tab[PROFILE_APP_ID].char2_handle,
            //                             sizeof(char_value_read2),
            //                             char_value_read2,
            //                             false);
            // }

            // if (notify_enabled_char3) {
            //     esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_APP_ID].gatts_if, 
            //                             gl_profile_tab[PROFILE_APP_ID].conn_id,
            //                             gl_profile_tab[PROFILE_APP_ID].char3_handle,
            //                             sizeof((uint8_t *)char_value_read3),
            //                             (uint8_t *)char_value_read3,
            //                             false);
            // }
            break;
        case ESP_GATTS_MTU_EVT:
            local_mtu = param->mtu.mtu;
            // printf("requested MTU size from client:%u\n",param->mtu.mtu);
        break;
        default:
            break;
    }
}

// Handles write callbacks and sends the response
void example_write_event_env(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp) {
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            // ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }
    
    //gatts_if registered complete, call cb handlers
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while(0);
}

// Initialize the BLE, set the MTU and start the read sensor data task
void bleInit(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bluedroid_init();
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bluedroid_enable();
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    
    ret = esp_ble_gatts_app_register(PROFILE_APP_ID);
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "app register error, error code = %x", ret);
        return;
    }
    
    ret = esp_ble_gatt_set_local_mtu(512);
    if (ret) {
        // ESP_LOGE(GATTS_TAG, "set local MTU failed, error code = %x", ret);
    }
    
    xTaskCreate(read_set_sensor_data_task, "read_set_sensor_data", 4 * 1024, NULL, 5, NULL);
}

// Deinitialize BLE
void ble_deinitialize(void)
{
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    // ESP_LOGE(GATTS_TAG, "BLE deinitialized");
}