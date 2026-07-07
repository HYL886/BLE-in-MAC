#ifndef __BOAT_H__
#define __BOAT_H__
#include "tl_common.h"
#include "drivers.h"
#include "ble.h"

void boat_set_type(u8 type);
void boat_respond_status(u8 *buf, u8 len);
void boat_request_status(void);
void boat_request_sport_data(void);
void boat_request_device_info(void);
void boat_set_user_info(void);
void boat_set_resister_incline(u8 resister, u8 incline);
void boat_ctl_start(void);
void boat_ctl_stop(void);
void boat_respond(u8 *buf, u8 len);
void boat_2AD9_respond(u8 cmd, u8 result);
void boat_2AD9_receive_callback(u8 *buf,u8 len);
void boat_proc(void);

#endif
