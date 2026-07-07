#ifndef __UART_H__
#define __UART_H__
#include "tl_common.h"
#include "drivers.h"

typedef void (*uart_callback_t)(u8 *buf, u8 len);

void uart_user_init(u32 baudrate);
void uart_user_irq_proc(void);
void uart_user_proc(void);
void uart_send_string(u8 *buf,u8 len);
void uart_set_callback(uart_callback_t p);
uart_callback_t uart_get_callback(void);


#endif
