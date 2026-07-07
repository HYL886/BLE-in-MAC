#include "hal.h"
#include "msg.h"


static hal_config_name_t hal_config_name;

static u8 hal_fcs(u8 *buf, u8 len)
{
    u8 i = 0,fcs = 0;
    for(i = 0; i < len; i++)
    {
        fcs ^= buf[i];
    }
    return fcs;
}

void hal_config_init(void)
{
}
static void hal_CD_01(u8 *buf, u8 len)
{
    u8 msg[20];

    msg[0] = 0x02;
    msg[1] = 0xCD;
    msg[2] = 0x01;
    msg[3] = hal_fcs(&msg[1],2);
    msg[4] = 0x03;
    msg_ble_send(msg,5);
}

static void hal_CD_02(u8 *buf, u8 len)
{
    u8 msg[20];

    msg[0] = 0x02;
    msg[1] = 0xCD;
    msg[2] = 0x02;
    msg[3] = hal_fcs(&msg[1],2);
    msg[4] = 0x03;
    msg_ble_send(msg,5);
}

static void hal_CE_01(u8 *buf, u8 len)
{
    u8 msg[20];
    u8 payload_len = 0;
    u8 *p = NULL;


    p = &buf[3];
    payload_len = len - 5;

    msg[0] = 0x02;
    msg[1] = 0xCE;
    msg[2] = 0x01;
    flash_erase_sector(HAL_CONFIG_NAME);
    if((payload_len <= 18) && (payload_len >= 3))
    {
        hal_config_name.len = payload_len;
        memcpy(hal_config_name.name,p,payload_len);
        hal_config_name.crc16 = crc16((u8 *)&hal_config_name,sizeof(hal_config_name_t)-2);
        flash_write_page(HAL_CONFIG_NAME,sizeof(hal_config_name_t),(u8 *)&hal_config_name);
        msg[3] = 0x01;
    }else{
        msg[3] = 0x00;
    }
    msg[4] = hal_fcs(&msg[1],3);
    msg[5] = 0x03;
    msg_ble_send(msg,6);
}

static void hal_CE_02(u8 *buf, u8 len)
{
    u8 msg[30];
	u8 over_len = 0;

    msg[0] = 0x02;
    msg[1] = 0xCE;
    msg[2] = 0x02;
    memset((u8 *)&hal_config_name,0x00,sizeof(hal_config_name));
    flash_read_page(HAL_CONFIG_NAME,sizeof(hal_config_name_t),(u8 *)&hal_config_name);
    if((hal_config_name.len > 0) 
            && (hal_config_name.crc16 == crc16((u8 *)&hal_config_name,sizeof(hal_config_name_t)-2)))
    {
        if(hal_config_name.len > 15) {
            over_len = hal_config_name.len - 15;
        }  
        memcpy(&msg[3],hal_config_name.name + over_len,hal_config_name.len - over_len);
        msg[hal_config_name.len + 3] = hal_fcs(&msg[1],hal_config_name.len - over_len + 2);
        msg[hal_config_name.len + 4] = 0x03;
        msg_ble_send(msg,hal_config_name.len - over_len + 5);
    }else{
        msg[3] = hal_fcs(&msg[1],2);
        msg[4] = 0x03;
        msg_ble_send(msg,5);
    }
}

u8 hal_get_name(u8 *buf)
{
    flash_read_page(HAL_CONFIG_NAME,sizeof(hal_config_name_t),(u8 *)&hal_config_name);
    if((hal_config_name.len > 0) 
            && (hal_config_name.crc16 == crc16((u8 *)&hal_config_name,sizeof(hal_config_name_t)-2)))
    {
        if((hal_config_name.len >= 3) && (hal_config_name.len <= 18))
        {
            memcpy(buf+2,hal_config_name.name,hal_config_name.len);
            buf[0] = hal_config_name.len + 1;
            return 1;
        }else{
            return 0;
        }
    }else{
        return 0;
    }
}

u8 hal_fc_test_check(u8 *buf, u8 len)
{
	u8 ret = 0;
	u16 event = 0;
    if(len >= 4)
    {
        if(len == 4)
        {
            event = buf[1] << 8;
        }else{
            event = (buf[1] << 8) + buf[2];
        }
    }
    //printf("event:0x%04X\n",event);
    switch(event)
    {
		case HAL_CD_01:
			hal_CD_01(buf,len);
			ret = 1;
			break;
			
		case HAL_CD_02:
			hal_CD_02(buf,len);
			ret = 1;
			break;
			
		case HAL_CE_01:
			hal_CE_01(buf,len);
			ret = 1;
			break;
			
		case HAL_CE_02:
			hal_CE_02(buf,len);
			ret = 1;
			break;
	}
   
    return ret;
}