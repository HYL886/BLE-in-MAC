#ifndef __FLASH_LOCK_H__
#define __FLASH_LOCK_H__
#include "tl_common.h"
#include "drivers.h"
#if(SYS_CONFIG_USE_SDK4_EN == 1)
void flash_lock(int deepRetWakeUp);
void flash_unlock(void);
#endif
#endif
