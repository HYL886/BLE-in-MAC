#ifndef __TREADMILL_H__
#define __TREADMILL_H__
#include "tl_common.h"
#include "drivers.h"
#include "ble.h"

void treadmill_set_type(u8 type);
void treadmill_request_status(void);
void treadmill_request_speed_range(void);
void treadmill_respond_status(u8 *buf, u8 len);
void treadmill_respond_speed_range(u8 *buf, u8 len);
void treadmill_2AD9_respond(u8 cmd,u8 result);
void treadmill_2AD9_receive_callback(u8 *buf,u8 len);
void treadmill_respond(u8 *buf, u8 len);
void treadmill_proc(void);

#endif
