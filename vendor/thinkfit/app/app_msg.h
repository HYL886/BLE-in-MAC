#ifndef __APP_MSG_H__
#define __APP_MSG_H__
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


void app_uart_receive(u8 *buf, u8 len);
void app_ble_receive(u8 *buf, u8 len);
void app_marach_receive(u8 *buf, u8 len);
void app_ftms_receive(u8 *buf, u8 len);
void ckit_bike_ADpackReceive(const u8 *mac, void * p);

#if(SYS_CONFIG_USE_SDK4_EN == 1)
void heartrate_set_value(u8 value);
u8 heartrate_get_value(void);
#endif
#endif
