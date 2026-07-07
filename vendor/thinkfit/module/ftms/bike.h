#ifndef __BIKE_H__
#define __BIKE_H__
#include "tl_common.h"
#include "drivers.h"
#include "ble.h"

void bike_set_type(u8 type);
void bike_respond_status(u8 *buf, u8 len);
void bike_request_status(void);
void bike_request_sport_data(void);
void bike_request_device_info(void);
void bike_set_user_info(void);
void bike_set_resister_incline(u8 resister, u8 incline);
void bike_ctl_start(void);
void bike_ctl_stop(void);
void bike_respond(u8 *buf, u8 len);
void bike_2AD9_respond(u8 cmd, u8 result);
void bike_2AD9_receive_callback(u8 *buf,u8 len);
void bike_proc(void);

#endif
