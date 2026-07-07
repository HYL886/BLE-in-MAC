#include "sm.h"
#include "uart.h"
#include "msg.h"
#include "ptl.h"

#if (SYS_CONFIG_USE_SDK4_EN == 1)
#include "flash_lock.h"
#include "hal_config.h"
#endif

extern void app_init(int deepRetWakeUp);
extern void app_deinit(void);
extern void app_proc(void);

static int sm_cur_sta = SM_STA_NORMAL;


int sm_get_cur_sta(void)
{
    return sm_cur_sta;
}

void sm_set_cur_sta(int sta)
{
    sm_cur_sta = sta;
}

void sm_init(int deepRetWakeUp)
{
#if (SYS_CONFIG_USE_SDK4_EN == 1)
    flash_lock(deepRetWakeUp);
#else
    ble_init(deepRetWakeUp);
#endif
	hal_config_init();
    uart_user_init(4096);
    ptl_init();
    sm_set_cur_sta(SM_STA_NORMAL);
    app_init(deepRetWakeUp);
    printf("System starting...\n");
}

void sm_deinit(void)
{
    app_deinit();
}

void sm_proc(void)
{
    // Call protocol and uart process in normal mode.
    if(sm_get_cur_sta() == SM_STA_NORMAL)
    {
        ptl_proc();
        uart_user_proc();
        app_proc();
    }
#if (SYS_CONFIG_USE_SDK4_EN == 0)
    ble_pm_proc();
#endif
}
