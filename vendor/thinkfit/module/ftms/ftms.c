#include "ftms.h"
#include "msg.h"
#include "treadmill.h"
#include "bike.h"
#include "boat.h"
#include "crtr.h"
#include "app_msg.h"

static u8 treadmill_unlock_key[7] = {0x05,0x0E,0x03,0x0D,0x02,0x03};

static u32 m_2acd_update_ticks = 0;
static int delay = 0;
static u8 ftms_start_flag = 0;
static u8 ftms_treadmill_unlock = 0;

void ftms_set_delay(void)
{
    delay = 0;
}

void ftms_start(void)
{
    ftms_start_flag = 1;
}

void ftms_stop(void)
{
    ftms_start_flag = 0;
}

#if(SYS_CONFIG_USE_SDK4_EN == 1)
int ftms_HFEDRead(u16 connHandle, rf_packet_att_t *p)
{
    return 1;
}

int ftms_HFEDWrite(u16 connHandle, rf_packet_att_t *p)
{
    rf_packet_att_t *pw = (rf_packet_att_t*)p;
    int len = pw->l2capLen - 3;
    u8 *buf = (u8 *)&pw->dat;
    if(len >= 8)
    {
        if((buf[0] & 0x0001) == 0x0001)
        {
            if(memcmp(&buf[2],treadmill_unlock_key,6) == 0)
            {
                ftms_treadmill_unlock = 1;
            }
        }
    }
	app_ftms_receive(buf,len);
	
    return 0;
}
#else
int ftms_HFEDRead(void * p)
{
    return 1;
}

int ftms_HFEDWrite(void * p)
{
    rf_packet_att_data_t *pw = (rf_packet_att_data_t*)p;
    int len = pw->l2cap - 3;
    u8 *buf = (u8 *)&pw->dat;
    if(len >= 8)
    {
        if((buf[0] & 0x0001) == 0x0001)
        {
            if(memcmp(&buf[2],treadmill_unlock_key,6) == 0)
            {
                ftms_treadmill_unlock = 1;
            }
        }
    }
	app_ftms_receive(buf,len);
	
    return 0;
}
#endif

extern void treadmill_att_init(void);
extern void bike_att_init(void);
extern void boat_att_init(void);
extern void crtr_att_init(void);
void ftms_init(void)
{
    m_2acd_update_ticks = clock_time();
#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
    treadmill_set_type(PARAMETER_TYPE_ID);
    treadmill_att_init();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
    crtr_set_type(PARAMETER_TYPE_ID);
    crtr_att_init();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
    bike_set_type(PARAMETER_TYPE_ID);
    bike_att_init();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
    boat_set_type(PARAMETER_TYPE_ID);
    boat_att_init();
#endif
}


void ftms_send(u8 *buf, u8 len)
{
    msg_ble_receive(buf, len);
}

void ftms_request_status(void)
{
#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
    treadmill_request_status();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
    crtr_request_status();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
    bike_request_status();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
    boat_request_status();
#endif
}

void ftms_respond(u8 *buf,u8 len)
{
#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
    treadmill_respond(buf,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
    crtr_respond(buf,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
    bike_respond(buf,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
    boat_respond(buf,len);
#endif
}

#if(SYS_CONFIG_USE_SDK4_EN == 1)
int ftms_2AD9_receive_callback(u16 connHandle, rf_packet_att_t *p)
{
    rf_packet_att_t *pw = (rf_packet_att_t*)p;
    int len = pw->l2capLen - 3;

#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
	/*
    if(ftms_treadmill_unlock == 0)
	{
		u8 buf[3];
		u8 *pcmd = &pw->dat;
		buf[0] = 0x80;
		buf[1] = pcmd[0];
		buf[2] = 0x05;
		bls_att_pushNotifyData(FITNESS_2AD9_DP_H,buf,3);
		return 1;
	}
	*/
    treadmill_2AD9_receive_callback((u8 *)&pw->dat,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
    crtr_2AD9_receive_callback((u8 *)&pw->dat,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
    bike_2AD9_receive_callback((u8 *)&pw->dat,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
    boat_2AD9_receive_callback((u8 *)&pw->dat,len);
#endif

    return 0;
}
#else
int ftms_2AD9_receive_callback(void *p)
{
    rf_packet_att_data_t *pw = (rf_packet_att_data_t*)p;
    int len = pw->l2cap - 3;

#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
	/*
    if(ftms_treadmill_unlock == 0)
	{
		u8 buf[3];
		u8 *pcmd = &pw->dat;
		buf[0] = 0x80;
		buf[1] = pcmd[0];
		buf[2] = 0x05;
		bls_att_pushNotifyData(FITNESS_2AD9_DP_H,buf,3);
		return 1;
	}
	*/
    treadmill_2AD9_receive_callback((u8 *)&pw->dat,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
    crtr_2AD9_receive_callback((u8 *)&pw->dat,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
    bike_2AD9_receive_callback((u8 *)&pw->dat,len);
#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
    boat_2AD9_receive_callback((u8 *)&pw->dat,len);
#endif

    return 0;
}
#endif

void ftms_proc(void)
{
    if(clock_time_exceed(m_2acd_update_ticks,500000))
    {
#if (SYS_CONFIG_USE_SDK4_EN == 1)
        if(blc_ll_getCurrentSlaveRoleNumber() == BLS_LINK_STATE_CONN)
        {
            //if(ftms_start_flag)
            {               
                if(delay ++ >= 2) // Delay 1s.
                {
                    delay = 2;
                    ftms_request_status();
                }
            }
        }else{
            ftms_treadmill_unlock = 0;
            ftms_start_flag = 0;
            delay = 0;
        }
        m_2acd_update_ticks = clock_time();
#else
        if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN)
      
        {
            if(ftms_start_flag)
            {               
                if(delay ++ >= 2) // Delay 1s.
                {
                    delay = 2;
                    ftms_request_status();
                }
            }
        }else{
            ftms_treadmill_unlock = 0;
            ftms_start_flag = 0;
            delay = 0;
        }
        m_2acd_update_ticks = clock_time();
#endif 
    }
#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
	treadmill_proc();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
	crtr_proc();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
	bike_proc();
#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
    boat_proc();
#endif
}

