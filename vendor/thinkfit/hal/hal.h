#ifndef __HAL_H__
#define __HAL_H__
#include "tl_common.h"
#include "drivers.h"
#include "hal_config.h"
#include "hal_uart.h"

typedef void (*msg_callback_t)(u8 *buf, u8 len);

u8 fcs_result(u8 *buf, u8 len);
void hal_init_ble_data(void);
void hal_set_ble_data(void);


#endif
