#include "app_msg.h"
#include "msg.h"
#include "ble.h"
#include "sm.h"
#include "uart.h"
#include "ptl.h"
#include "ftms.h"
#include "hal.h"
#include "heartrate_service.h"

#if(SYS_CONFIG_USE_SDK4_EN == 1)
static u8 c_heartrate = 0;
void heartrate_set_value(u8 value)
{
	c_heartrate = value;
}

u8 heartrate_get_value(void)
{
	return c_heartrate;
}
#endif

static u8 msg_respond_50_00(u8 *buf, u8 len, msg_callback_t pfunc)
{
	u8 ret = 0;
	
	if((len == 5) && (buf[0] == 0x02) && (buf[1] == 0x50) && (buf[2] == 0x00))
	{
		u8 msg[20];
		msg[0] = 0x02;
		msg[1] = 0x50;
		msg[2] = 0x00;
		msg[3] = PARAMETER_BRAND_ID;
		msg[4] = PARAMETER_TYPE_ID;
		msg[5] = PARAMETER_MODE_ID & 0xff;
		msg[6] = (PARAMETER_MODE_ID >> 8) & 0xff;
		msg[7] = msg_fcs_result(&msg[1],6);
		msg[8] = 0x03;
		pfunc(msg,9);
		ret = 1;
	}
	
	return ret;
}

void app_uart_receive(u8 *buf, u8 len)
{
	if(msg_respond_50_00(buf,len,msg_uart_send) == 0)
	{
#if (SYS_CONFIG_USE_SDK4_EN == 1)
		if((buf[0] == 0x02)&&(buf[1] == 0x42)&&((buf[2] == 0x02)||(buf[2]==0x03)))
		{
			if(heartrate_get_value() != 0)
			{
				buf[8] = heartrate_get_value();
			}
		}
#endif
		msg_ble_send(buf,len);
		ftms_respond(buf,len);
        heartrate_service_proc(buf,len);
	}
}



void app_ble_receive(u8 *buf, u8 len)
{
	if(msg_respond_50_00(buf,len,msg_ble_send) == 0)
	{
		msg_uart_send(buf,len);
	}
}

void app_marach_receive(u8 *buf, u8 len)
{
    if(len == 5)
    {
        if((buf[0] == 0xAA)
                &&(buf[1] == 0x01)
                &&(buf[2] == 0x00)
                &&(buf[3] == 0x01)
                &&(buf[4] == 0x55))
        {
			msg_uart_send(buf,5);
            bls_att_pushIndicateData(MRK_0000_DP_H,buf,len);
        }
    }
}

void app_ftms_receive(u8 *buf, u8 len)
{
	
}

static void ckit_bike_remote_button(u8 click_event,msg_callback_t pfunc)
{
    u8 msg[20];
    msg[0] = 0x02;
    msg[1] = 0x44;
    msg[2] = 0x06;
    msg[3] = click_event;
    msg[4] = msg_fcs_result(&msg[1],3);
    msg[5] = 0x03;
    pfunc(msg,6);
}

#define BLE_KEY_DEVICE_MAX 4
#define BLE_KEY_BASE_INFO_LEN 3
#define BLE_KEY_INVALID_COUNT 0xffffffff

typedef struct {
	u8 used;
	u8 mac[6];
	u8 base_info[BLE_KEY_BASE_INFO_LEN];
	u32 last_count;
	u8 last_key;
	u32 timestamp;
} ble_key_device_t;

static ble_key_device_t device_list[BLE_KEY_DEVICE_MAX];
static u8 device_count = 0;

static u8 ble_key_mac_equal(const u8 *mac1, const u8 *mac2)
{
	u8 i;

	for(i = 0; i < 6; i++)
	{
		if(mac1[i] != mac2[i])
		{
			return 0;
		}
	}

	return 1;
}

static void ble_key_record_reset(ble_key_device_t *device, const u8 *mac, const u8 *base_info, u32 now)
{
	u8 i;

	device->used = 1;
	for(i = 0; i < 6; i++)
	{
		device->mac[i] = mac[i];
	}

	for(i = 0; i < BLE_KEY_BASE_INFO_LEN; i++)
	{
		device->base_info[i] = base_info[i];
	}

	device->last_count = BLE_KEY_INVALID_COUNT;
	device->last_key = 0;
	device->timestamp = now;
}

static u8 ble_key_base_info_same(ble_key_device_t *device, const u8 *base_info)
{
	u8 i;

	for(i = 0; i < BLE_KEY_BASE_INFO_LEN; i++)
	{
		if(device->base_info[i] != base_info[i])
		{
			return 0;
		}
	}

	return 1;
}

static ble_key_device_t *ble_key_get_device(const u8 *mac, const u8 *base_info, u32 now)
{
	u8 i;
	u8 oldest = 0;

	if(mac == NULL)
	{
		return NULL;
	}

	for(i = 0; i < BLE_KEY_DEVICE_MAX; i++)
	{
		if(device_list[i].used && ble_key_mac_equal(device_list[i].mac, mac))
		{
			if(!ble_key_base_info_same(&device_list[i], base_info))
			{
				ble_key_record_reset(&device_list[i], mac, base_info, now);
			}
			else
			{
				device_list[i].timestamp = now;
			}
			return &device_list[i];
		}
	}

	for(i = 0; i < BLE_KEY_DEVICE_MAX; i++)
	{
		if(!device_list[i].used)
		{
			ble_key_record_reset(&device_list[i], mac, base_info, now);
			device_count++;
			return &device_list[i];
		}
	}

	for(i = 1; i < BLE_KEY_DEVICE_MAX; i++)
	{
		if(device_list[i].timestamp < device_list[oldest].timestamp)
		{
			oldest = i;
		}
	}

	ble_key_record_reset(&device_list[oldest], mac, base_info, now);
	return &device_list[oldest];
}

void ckit_bike_ADpackReceive(const u8 *mac, void * p)
{
	ble_key_device_t *device = NULL;
	u8 *pw = (u8*)p;
    //printf("%d,%d\n",pw[3],pw[4]);
	if(msg_fcs_result(&pw[0],5) == pw[5])
	{
		device = ble_key_get_device(mac, pw, clock_time());
		if((device != NULL) && (device->last_count != pw[3]))
		{
			switch(pw[4])
			{
				case 1:
					// ckit_bike_key_switch(0);
					break;
					
				case 2:
					// ckit_bike_previous();
                    ckit_bike_remote_button(0xf2,msg_uart_send);
					break;
					
				case 3:
                    ckit_bike_remote_button(0xf1,msg_uart_send);
					// ckit_bike_next();
					break;
					
				default:
					break;
			}
			device->last_count = pw[3];
			device->last_key = pw[4];
		}
	}
}
