#ifndef __MSG_H__
#define __MSG_H__
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#define MSG_MAX_LEN  20

enum {
    THINKFIT_41_02 = 0x4102,
    THINKFIT_41_04 = 0x4104,

    THINKFIT_42_00 = 0x4200,
    THINKFIT_42_01 = 0x4201,
    THINKFIT_42_02 = 0x4202,
    THINKFIT_42_03 = 0x4203,
    THINKFIT_42_14 = 0x4214,
    THINKFIT_42_15 = 0x4215,

    THINKFIT_43_01 = 0x4301,
    THINKFIT_44_01 = 0x4401,
    THINKFIT_44_02 = 0x4402,
    THINKFIT_44_03 = 0x4403,
    THINKFIT_44_04 = 0x4404,
    THINKFIT_44_05 = 0x4405,
    THINKFIT_44_0A = 0x440A,
    THINKFIT_44_0B = 0x440B,

    THINKFIT_50_01 = 0x5001,
    THINKFIT_50_02 = 0x5002,
    THINKFIT_50_03 = 0x5003,
    THINKFIT_50_04 = 0x5004,
    THINKFIT_51_00 = 0x5100,
    THINKFIT_51_01 = 0x5101,
    THINKFIT_51_02 = 0x5102,
    THINKFIT_51_03 = 0x5103,
    THINKFIT_51_04 = 0x5104,
    THINKFIT_51_05 = 0x5105,
    THINKFIT_51_06 = 0x5106,
    THINKFIT_51_09 = 0x5109,
    THINKFIT_51_0A = 0x510A,
    THINKFIT_53_00 = 0x5300,
    THINKFIT_53_01 = 0x5301,
    THINKFIT_53_02 = 0x5302,
    THINKFIT_53_03 = 0x5303,
    THINKFIT_53_09 = 0x5309,
    THINKFIT_53_0A = 0x530A,
    THINKFIT_53_53 = 0x5353,
};

void msg_uart_receive(u8 *buf, u8 len);
void msg_uart_send(u8 *buf, u8 len);
void msg_ble_receive(u8 *buf, u8 len);
void msg_ble_send(u8 *buf, u8 len);
u8 msg_fcs_result(u8 *buf, u8 len);

#if(SYS_CONFIG_USE_SDK4_EN == 1)
int MRK_HR_Write(u16 connHandle, rf_packet_att_t *p);
#else
int MRK_HR_Write(void *p);
#endif
#if (SYS_CONFIG_USE_SDK4_EN == 0)
int ble_com_receive_callback(void *p);
#endif

#endif
