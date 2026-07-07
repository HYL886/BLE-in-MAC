#ifndef __APP_ATT_H__
#define __APP_ATT_H__
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "sm.h"

void att_set_2ACC_data(u8 *buf, u8 len);
void att_set_2AD3_data(u8 *buf, u8 len);
void att_set_2AD4_data(u8 *buf, u8 len);
void att_set_2AD5_data(u8 *buf, u8 len);
void att_set_2AD6_data(u8 *buf, u8 len);
void att_set_2AD7_data(u8 *buf, u8 len);
void att_set_2AD8_data(u8 *buf, u8 len);

#endif
