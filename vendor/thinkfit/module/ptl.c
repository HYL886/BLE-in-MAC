#include "ptl.h"
#include "uart.h"
#include "msg.h"
#include "ftms.h"


static u8 ptl_sta = PTL_STA_NORMAL;

enum {
    PTL_BAUDRATE_4096 = 0,
    PTL_BAUDRATE_9600,
    PTL_BAUDRATE_19200,
    PTL_BAUDRATE_38400,
    PTL_BAUDRATE_115200,
    PTL_BAUDRATE_MAX,
};
static u8 ptl_baudrate = PTL_BAUDRATE_4096;

enum {
    PTL_VERIFY_START = 0,
    PTL_VERIFY_RUNNING,
    PTL_VERIFY_SUCCESS,
};

static u8 ptl_verify = PTL_VERIFY_START;
static u32 ptl_verify_ticks = 0;

static u8 tf_verify_cmd[] = {0x02,0x50,0x00,0x50,0x03};

void ptl_init(void)
{
#if (SYS_USE_MODU_ONLY == 1)
    ptl_sta = PTL_STA_VERIFY;
    ptl_baudrate = PTL_BAUDRATE_4096;
    ptl_verify = PTL_VERIFY_START;
#elif (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
    ptl_sta = PTL_STA_VERIFY;
    ptl_baudrate = PTL_BAUDRATE_4096;
    ptl_verify = PTL_VERIFY_START;
#else
	ptl_sta = PTL_STA_NORMAL;
	uart_set_callback(msg_uart_receive);
	ftms_init();
#endif
}
static u32 ptl_verify_get_baudrate(u8 baudrate_index)
{
    switch(baudrate_index)
    {
        case PTL_BAUDRATE_4096:
            return 4096;
        case PTL_BAUDRATE_9600:
            return 9600;
        case PTL_BAUDRATE_19200:
            return 19200;
        case PTL_BAUDRATE_38400:
            return 38400;
        case PTL_BAUDRATE_115200:
            return 115200;
    }
    return 9600;
}

static void ptl_verify_tf_callback(u8 *buf, u8 len)
{
    u8 i = 0,fcs = 0;

    printf("verify:len:%d\n",len);
    for(i = 0; i < len; i++)
    {
        printf(" %02X",buf[i]);
    }
    printf("\n");
    ptl_verify = PTL_VERIFY_START;
    if((buf[0] == 0x02) && (buf[1] == 0x50) && (buf[len - 1] == 0x03) && (len > 7))
    {
        for(i = 1; i < len - 2; i++)
        {
            fcs ^= buf[i];
        }
        if(fcs == buf[len - 2])
        {
            ptl_verify = PTL_VERIFY_SUCCESS;
        }
    }

    if(ptl_verify == PTL_VERIFY_SUCCESS)
    {
        uart_set_callback(msg_uart_receive);
        ptl_sta = PTL_STA_NORMAL;
		ftms_init();
    }
}

static void ptl_verify_thinkfit(void)
{
    uart_user_init(ptl_verify_get_baudrate(ptl_baudrate++));
    uart_send_string(tf_verify_cmd,sizeof(tf_verify_cmd));
    uart_set_callback(ptl_verify_tf_callback);
}

static void ptl_verify_proc(void)
{
    if(ptl_verify == PTL_VERIFY_START)
    {
        printf("baudrate:%d\n",ptl_baudrate);
        ptl_verify = PTL_VERIFY_RUNNING;
        ptl_verify_thinkfit();
        ptl_verify_ticks = clock_time(); 
    }else if(ptl_verify == PTL_VERIFY_RUNNING)
    {
        if(clock_time_exceed(ptl_verify_ticks, 200000))
        {
            if(ptl_baudrate >= PTL_BAUDRATE_MAX)
            {
                ptl_baudrate = PTL_BAUDRATE_4096;
            }
            ptl_verify = PTL_VERIFY_START;
        }
    }
}

u8 ptl_get_sta(void)
{
    return ptl_sta;
}

void ptl_proc(void)
{
    switch(ptl_sta)
    {
        case PTL_STA_VERIFY:
            ptl_verify_proc();
            break;
        case PTL_STA_NORMAL:
            ftms_proc();
            break;
    }
}
