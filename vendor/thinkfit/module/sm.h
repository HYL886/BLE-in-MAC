#ifndef __SM_H__
#define __SM_H__
#include "tl_common.h"
#include "stack/ble/ble.h"
#include "drivers.h"
#include "ble.h"

enum {
    SM_STA_NORMAL = 0,
    SM_STA_OTA,
};

void sm_disable_suspend(u8 disable);
void sm_init(int deepRetWakeUp);
void sm_deinit(void);
void sm_proc(void);
int sm_get_cur_sta(void);
void sm_set_cur_sta(int sta);

#endif
