#ifndef BLE_MAIN_H_
#define BLE_MAIN_H_

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

extern char *rec_value4;
extern size_t rec_value4_length;

void bleInit(void);
void ble_deinitialize(void);

#endif /*BLE_MAIN_H_*/