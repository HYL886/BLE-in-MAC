#ifndef __BLE_H__
#define __BLE_H__
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "sm.h"

#if (SYS_CONFIG_USE_SDK4_EN == 1)

#include "vendor/common/blt_led.h"
#include "vendor/common/blt_common.h"
#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"

#define bls_att_pushNotifyData(a,b,c)	blc_gatt_pushHandleValueNotify(conn_dev_list[1].conn_handle,a,b,c)
#define bls_att_pushIndicateData(a,b,c)	blc_gatt_pushHandleValueIndicate(conn_dev_list[1].conn_handle,a,b,c)

#define BLE_POWEROFF_ENTER_DEEP_TIME          3   //3 s
#define BLS_LINK_STATE_ADV			0
#define BLS_LINK_STATE_CONN			1

int ble_com_receive_callback(u16 connHandle, rf_packet_att_t *p);
void ble_init(int deepRetWakeUp);
void ble_pm_proc(void);
void ble_set_power_off(void);
void ble_heartrate_set_hande(u8 handle);

#else
#define BLE_POWERON_ENTER_DEEP_TIME           30 //180 s
#define BLE_POWEROFF_ENTER_DEEP_TIME          3   //3 s
#define BLE_OTA_ENTER_DEEP_TIME               240 //240 s

void ble_keep_alive(void);
u32 ble_get_alive_tick(void);
void ble_init(int deepRetWakeUp);
void ble_pm_proc(void);
void ble_set_power_on(void);
void ble_set_power_off(void);
void ble_set_ota_tick(void);
void ble_set_sleep_tick(u32 tick);
u32 ble_get_sleep_tick(void);
void ble_set_scanRsp(u8 *pid,u8* model);

#endif

#endif
