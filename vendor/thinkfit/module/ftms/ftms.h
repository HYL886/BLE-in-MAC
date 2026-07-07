#ifndef __FTMS_H__
#define __FTMS_H__
#include "tl_common.h"
#include "drivers.h"
#include "ble.h"
#include "app_att.h"

void ftms_init(void);
void ftms_proc(void);

void ftms_start(void);
void ftms_stop(void);
void ftms_set_delay(void);

void ftms_request_speed_range(void);
void ftms_request_incline_range(void);
void ftms_respond(u8 *buf,u8 len);
void ftms_send(u8 *buf, u8 len);

#if(SYS_CONFIG_USE_SDK4_EN == 1)
int ftms_2AD9_receive_callback(u16 connHandle, rf_packet_att_t *p);
#else
int ftms_2AD9_receive_callback(void *p);
#endif

#endif
