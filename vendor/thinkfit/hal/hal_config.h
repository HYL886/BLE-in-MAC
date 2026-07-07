#ifndef __HAL_CONFIG_H__
#define __HAL_CONFIG_H__
#include "tl_common.h"
#include "drivers.h"

#define HAL_CONFIG_ADR      0x7E000
#define HAL_CONFIG_NAME     0x7D000

enum {
    HAL_CC_01 = 0xCC01,
    HAL_CC_02 = 0xCC02,
	HAL_CC_03 = 0xCC03,
	HAL_CD_01 = 0xCD01,
    HAL_CD_02 = 0xCD02,
    HAL_CE_01 = 0xCE01,
    HAL_CE_02 = 0xCE02,
};

typedef struct _hal_config_name_{
    u8  len;
    u8  name[20];
    u16 crc16;
}hal_config_name_t;
void hal_config_init(void);
u8 hal_get_name(u8 *buf);
u8 hal_fc_test_check(u8 *buf, u8 len);

#endif
