#include "heartrate_service.h"
#include "app_config.h"
#include "ble.h"

static u32 heartrate_service_send_ticks = 0;

void heartrate_service_proc(u8 *buf,u8 len)
{
#if (FUNCTION_TYPE != FUNCTION_TYPE_TREADMILL)
    if(((buf[2]==0x02)||(buf[2]==0x03))&&(len>=15))
    {
        u8 heart_buf[2]={0};
        heart_buf[0] = 0x00;
        heart_buf[1] = buf[8];
#if(SYS_CONFIG_USE_SDK4_EN == 1)
        if(clock_time_exceed(heartrate_service_send_ticks,950000))
#else
        if(clock_time_exceed(heartrate_service_send_ticks,800000))
#endif
        {
            if(BLE_SUCCESS == bls_att_pushNotifyData(HEARERATE_2A37_DP_H,heart_buf,sizeof(heart_buf)))
            {

            }
            heartrate_service_send_ticks = clock_time();
        }
    }
#endif
}
