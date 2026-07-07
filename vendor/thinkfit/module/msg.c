#include "msg.h"
#include "ble.h"
#include "sm.h"
#include "uart.h"
#include "ptl.h"
#include "ftms.h"
#include "app_msg.h"
#include "hal_config.h"

#define MSG_DEBUG 0

u8 msg_fcs_result(u8 *buf, u8 len)
{
    u8 i = 0,fcs = 0;
    if(len > 0)
    {
        for(i = 0; i < len; i++)
        {
            fcs ^= buf[i];
        }
    }
    return fcs;
}


void msg_uart_send(u8 *buf, u8 len)
{
	if(len <= MSG_MAX_LEN && ptl_get_sta() == PTL_STA_NORMAL)
    {
        uart_send_string(buf,len);
    }
}

void msg_ble_send(u8 *buf, u8 len)
{
	bls_att_pushNotifyData(COMM_FFF1_DP_H,buf,len);
}

void msg_uart_receive(u8 *buf, u8 len)
{
#if MSG_DEBUG
	int i = 0;
    printf("uart:\n");
    for(i = 0; i < len; i++)
    {
        printf(" %02X",buf[i]);
    }
    printf("\n");
#endif

    if(len <= MSG_MAX_LEN)
    {
		app_uart_receive(buf,len);
    }
}

void msg_ble_receive(u8 *buf, u8 len)
{
#if MSG_DEBUG
    int i = 0;
    printf("ble:\n");
    for(i = 0; i < len; i++)
    {
        printf(" %02X",buf[i]);
    }
    printf("\n");
#endif
	if(hal_fc_test_check(buf,len) == 1)
	{
		return;	
	}	
    if(len <= MSG_MAX_LEN && ptl_get_sta() == PTL_STA_NORMAL)
    {
		app_ble_receive(buf,len);	
    }
}

#if(SYS_CONFIG_USE_SDK4_EN == 1)
int MRK_HR_Write(u16 connHandle, rf_packet_att_t *p)
{
    rf_packet_att_t *pw = p;
    int len = pw->l2capLen - 3;
    u8 *buf;
    buf = (u8 *)&pw->dat;
    app_marach_receive(buf,len);
	
    return 0;
}
#else
int MRK_HR_Write(void *p)
{
    rf_packet_att_data_t *pw = (rf_packet_att_data_t*)p;
    int len = pw->l2cap - 3;
    u8 *buf;
    buf = (u8 *)&pw->dat;
    ble_keep_alive();
    app_marach_receive(buf,len);
	
    return 0;
}
#endif

#if (SYS_CONFIG_USE_SDK4_EN == 0)
int ble_com_receive_callback(void *p)
{
    rf_packet_att_data_t *pw = (rf_packet_att_data_t*)p;
    int len = pw->l2cap - 3;

    ble_keep_alive();
    ftms_set_delay();
    if(hal_fc_test_check((u8 *)&pw->dat,len) == 0)
    {
        msg_ble_receive((u8 *)&pw->dat,len);
    }
    return 0;
}
#endif
