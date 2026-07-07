#include "uart.h"
#include "msg.h"
#include "sm.h"
#include "hal.h"

#define UART_DATA_LEN           20
#define UART_RECEIVE_BUF_LEN    20
#define UART_SEND_DATA_LIST_LEN 5

static volatile unsigned int  uart_ndmarx_cnt = 0;
static volatile unsigned char uart_ndmarx_index = 0;
static volatile unsigned char uart_ndmatx_index = 0;

static u8 uart_receive_flag = 0;
static u8 uart_receive_buf[UART_RECEIVE_BUF_LEN];
static u8 uart_receive_buf_len = 0;

static __attribute__((aligned(4))) u8 rec_buff[UART_DATA_LEN];

static uart_callback_t pcallback = NULL;

struct _uart_send_data_list_{
    u8 buf[UART_DATA_LEN];
    u8 len;
    u8 available;
};
static struct _uart_send_data_list_ uart_send_data_list[UART_SEND_DATA_LIST_LEN];
static volatile u8 uart_list_head = 0,uart_list_tail = 0;

static u8 uart_fcs(u8 *buf, u8 len)
{
    u8 i = 0,fcs = 0;
    for(i = 0; i < len; i++)
    {
        fcs ^= buf[i];
    }
    return fcs;
}

static u8 uart_check(u8 *buf,u8 len)
{
    if((buf[0] == 0x02) 
            && (buf[len - 1] == 0x03) 
            && (uart_fcs(&buf[1],len - 3) == buf[len - 2]))
    {
        return 1;
    }else{
        return 0;
    }
}

void uart_user_init(u32 baudrate)
{
    memset(uart_send_data_list,0x00,sizeof(uart_send_data_list));
    uart_dma_enable(0, 0);
    uart_irq_enable(0, 0);
    uart_gpio_set(UART_GPIO_TX, UART_GPIO_RX);
    uart_reset();
    uart_ndmarx_cnt = 0;
    uart_ndmarx_index = 0;
    uart_ndmatx_index = 0;
    uart_init_baudrate(baudrate,CLOCK_SYS_CLOCK_HZ,PARITY_NONE, STOP_BIT_ONE);
    irq_disable_type(FLD_IRQ_DMA_EN);
    dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 0);
    uart_irq_enable(1, 0);
    uart_ndma_irq_triglevel(1, 0);
}

void uart_user_irq_proc(void)
{
    static u32 uart_bytes_ticks = 0;
    static u8 uart_ndma_irqsrc;
    if(clock_time_exceed(uart_bytes_ticks,10000))
    {
        uart_ndmarx_cnt = 0;
    }
    uart_bytes_ticks = clock_time();
	uart_ndma_irqsrc = uart_ndmairq_get();
	if(uart_ndma_irqsrc){
        rec_buff[uart_ndmarx_cnt] = reg_uart_data_buf(uart_ndmarx_index);
        uart_ndmarx_index++;
        uart_ndmarx_index &= 0x03;
        if((uart_ndmarx_cnt == 0) && (rec_buff[uart_ndmarx_cnt] != 0x02))
        {
            return;
        }
        if((rec_buff[uart_ndmarx_cnt] == 0x03) && (uart_check(rec_buff,uart_ndmarx_cnt + 1) == 1))
        {
            uart_receive_buf_len = uart_ndmarx_cnt + 1;
            memcpy(uart_receive_buf,rec_buff,uart_receive_buf_len);
            uart_ndmarx_cnt = 0;
            uart_receive_flag = 1;
        }else{
            uart_ndmarx_cnt++;
        }

        if(uart_ndmarx_cnt >= UART_DATA_LEN)
        {
            uart_ndmarx_cnt = 0;
        }
    }
}

static void uart_send_out(u8 *str,u8 len)
{
    // Uart just available in normal mode.
    if(sm_get_cur_sta() != SM_STA_NORMAL)
    {
        return;
    }
    if(len > UART_DATA_LEN || len < 4)
        return;

    u32 uart_send_tick = 0;
    uart_send_tick = clock_time();
    while(uart_tx_is_busy())
    {
        if(clock_time_exceed(uart_send_tick,20000))
        {
            printf("uart send timeout\n");
            return;
        }
    }
    for(u8 i = 0;i < len; i++){
        while((reg_uart_buf_cnt>>4)>7);
	    reg_uart_data_buf(uart_ndmatx_index) = str[i];
	    uart_ndmatx_index++;
	    uart_ndmatx_index &= 0x03;
    }
}

void uart_send_string(u8 *buf,u8 len)
{
    // Uart just available in normal mode.
    if(sm_get_cur_sta() != SM_STA_NORMAL)
    {
        return;
    }
    if(len > UART_DATA_LEN || len < 4)
        return;

    u8 index = 0;
    for(index = 0; index < UART_SEND_DATA_LIST_LEN; index++)
    {
        if((uart_send_data_list[index].available == 1) 
                && (uart_send_data_list[index].buf[0] == buf[0])
                && (uart_send_data_list[index].buf[1] == buf[1])
                && (uart_send_data_list[index].buf[2] == buf[2]))
        {
            memcpy(uart_send_data_list[index].buf,buf,len);
            uart_send_data_list[index].len = len;
            uart_send_data_list[index].available = 1;
            return;
        }
    }

    if(uart_list_tail >= UART_SEND_DATA_LIST_LEN)
    {
        uart_list_tail = 0;
    }
    if(uart_send_data_list[uart_list_tail].available == 0)
    {
        memcpy(uart_send_data_list[uart_list_tail].buf,buf,len);
        uart_send_data_list[uart_list_tail].len = len;
        uart_send_data_list[uart_list_tail].available = 1;
        uart_list_tail++;
    }
}

static void uart_user_data_handler()
{
    if(uart_receive_buf_len <= UART_DATA_LEN)
    {
        if(pcallback != NULL)
        {
            (*pcallback)(uart_receive_buf,uart_receive_buf_len);
        }
    }
}

void uart_set_callback(uart_callback_t p)
{
    pcallback = p;
}

uart_callback_t uart_get_callback(void)
{
    return pcallback;
}

void uart_user_proc(void)
{
    // For uart send
    static u32 uart_send_data_list_ticks = 0;
    static u32 timeout_value = 200000;
    if(clock_time_exceed(uart_send_data_list_ticks,timeout_value))
    {
        uart_send_data_list_ticks = clock_time();
        if(uart_list_head >= UART_SEND_DATA_LIST_LEN)
        {
            uart_list_head = 0;
        }
        if(uart_send_data_list[uart_list_head].available == 1)
        {
            uart_send_out(uart_send_data_list[uart_list_head].buf,uart_send_data_list[uart_list_head].len);
            uart_send_data_list[uart_list_head].available = 0;
            uart_list_head++;
            timeout_value = 200000;
        }else{
            u8 index = 0;
            for(index = 0; index < UART_SEND_DATA_LIST_LEN; index++)
            {
                if(uart_send_data_list[index].available == 1) 
                {
                    uart_list_head = index;
                    break;
                }
            }
            timeout_value = 1000;
        }
    }

    // For uart receive
    if(uart_receive_flag == 1)
    {
        uart_user_data_handler();
        uart_receive_flag = 0;
    }
}
