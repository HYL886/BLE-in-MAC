#include "irq.h"
#include "uart.h"

extern void app_irq_proc(void);

_attribute_ram_code_ void irq_handler(void)
{
#if(SYS_CONFIG_USE_SDK4_EN == 1)
    blc_sdk_irq_handler();
#else
    irq_blt_sdk_handler();
#endif
    uart_user_irq_proc();
    app_irq_proc();
}
