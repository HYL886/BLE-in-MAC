#ifndef __CRTR_H__
#define __CRTR_H__
#include "tl_common.h"
#include "drivers.h"
#include "ble.h"

void crtr_set_type(u8 type);
void crtr_respond_status(u8 *buf, u8 len);
void crtr_request_status(void);
void crtr_request_sport_data(void);
void crtr_request_device_info(void);
void crtr_set_user_info(void);
void crtr_set_resister_incline(u8 resister, u8 incline);
void crtr_ctl_start(void);
void crtr_ctl_stop(void);
void crtr_respond(u8 *buf, u8 len);
void crtr_2AD9_respond(u8 cmd, u8 result);
void crtr_2AD9_receive_callback(u8 *buf,u8 len);
void crtr_proc(void);

#endif
