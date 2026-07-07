#include "treadmill.h"
#include "msg.h"
#include "ble.h"
#include "ftms.h"

enum {
    FTMS_TREADMILL_RSP_SUCCESS = 0x01,
    FTMS_TREADMILL_RSP_NOT_SUPP = 0x02,
    FTMS_TREADMILL_RSP_INV_PARA = 0x03,
    FTMS_TREADMILL_RSP_FAILED = 0x04,
    FTMS_TREADMILL_RSP_NOT_PERM = 0x05,
};
enum {
    FTMS_TREADMILL_REQ_CTRL = 0x00,
    FTMS_TREADMILL_REQ_RESET = 0x01,
    FTMS_TREADMILL_SET_SPEED = 0x02,
    FTMS_TREADMILL_SET_INCLINE = 0x03,
    FTMS_TREADMILL_CTL_START = 0x07,
    FTMS_TREADMILL_CTL_STOP = 0x08,
};

extern void att_set_2ACC_data(u8 *buf, u8 len);
extern void att_set_2AD3_data(u8 *buf, u8 len);
extern void att_set_2AD4_data(u8 *buf, u8 len);
extern void att_set_2AD5_data(u8 *buf, u8 len);

static u8 treadmill_2ACC_Data[8] = {0x1c,0x16,0x00,0x00,0x03,0x00,0x00,0x00};
static u8 treadmill_2AD3_Data[2] = {0x01,0x01};
static u8 treadmill_2AD4_Data[6] = {0x00,0x00,0x00,0x00,0x0a,0x00};
static u8 treadmill_2AD5_Data[6] = {0x00,0x00,0x00,0x00,0x0a,0x00};
static u8 treadmill_2ADA_Data[3] = {0x00,0x00,0x00};

static u16 cur_speed = 0,cur_incline = 0,cur_energy = 0;
static u8 cur_unit = 0;
static u16 old_speed = 100,old_incline = 0;
static u16 new_target_speed = 100,new_target_incline = 0;
static u32 new_target_speed_ticks = 0,new_target_incline_ticks = 0;

static u8 push_buf[MSG_MAX_LEN];
static u8 push_buf2[MSG_MAX_LEN];
static u8 treadmill_2ACD_send_flag = 0;
static u8 treadmill_2ACD_send_len = 0;
static u8 treadmill_2ACD_send_len2 = 0;
static u32 treadmill_2ACD_send_ticks = 0;

static u8 treadmill_2ADA_send_flag = 0;
static u8 treadmill_2ADA_send_len = 0;
static u32 treadmill_2ADA_send_ticks = 0;

static u16 elevation_gain = 0;

static void treadmill_elevation_gain(u32 distance,u16 incline)
{
    static u32 start_distance = 0;

    if(distance > start_distance)
    {
        elevation_gain += (incline *(distance - start_distance))/100;
        start_distance = distance;
    }else{
        if(distance < 100)
        {
            elevation_gain = 0;
            start_distance = 0;
        }
    }
}

void treadmill_respond_status(u8 *buf, u8 len)
{
    static u8 old_status = 0;
    memset(push_buf,0x00,MSG_MAX_LEN);
    memset(push_buf2,0x00,MSG_MAX_LEN);
    push_buf[0] = 0x8C;
    push_buf[1] = 0x05;
    push_buf2[0] = 0x10;
    push_buf2[1] = 0x20;
    if(len >= 4)
    {
        switch(buf[2])
        {
            case 0x01:
            case 0x03:
            case 0x04:
                if(len >= 15)
                {
                    cur_speed = buf[3]*10;
                    if(cur_unit)
                    {
                        cur_speed = cur_speed * 161 / 100;
                    }
                    if(old_speed != cur_speed)
                    {
                        old_speed = cur_speed;
                        new_target_speed_ticks = clock_time();
                    }else{
                        if(clock_time_exceed(new_target_speed_ticks,1000000))
                        {
                            if((new_target_speed != cur_speed) && (buf[2] == 0x03))
                            {
                                new_target_speed = cur_speed;
                                if(cur_speed >= 100)
                                {
                                    treadmill_2ADA_Data[0] = 0x05;
                                    treadmill_2ADA_Data[1] = cur_speed & 0xff;
                                    treadmill_2ADA_Data[2] = (cur_speed>>8) & 0xff;
                                    treadmill_2ADA_send_len = 3;
                                    bls_att_pushNotifyData(FITNESS_2ADA_DP_H,treadmill_2ADA_Data,treadmill_2ADA_send_len);
                                }
                            }
                        }
                    }
                    push_buf[2] = cur_speed & 0xff;
                    push_buf[3] = (cur_speed >> 8) & 0xff;

                    u32 cur_distance = buf[7] + (buf[8] << 8);
                    if(cur_distance & 0x8000)
                    {
                        cur_distance = (cur_distance & 0x7fff) * 10;
                    }
                    if(cur_unit)
                    {
                        cur_distance = cur_distance * 161 / 100;
                    }
                    push_buf[4] = cur_distance & 0xff;
                    push_buf[5] = (cur_distance >> 8) & 0xff;
                    push_buf[6] = (cur_distance >> 16) & 0xff;

                    cur_incline = buf[4] * 10;
                    treadmill_elevation_gain(cur_distance,cur_incline);

                    if(old_incline != cur_incline)
                    {
                        old_incline = cur_incline;
                        new_target_incline_ticks = clock_time();
                    }else{
                        if(clock_time_exceed(new_target_incline_ticks,1000000))
                        {
                            if((new_target_incline != cur_incline) && (buf[2] == 0x03))
                            {
                                new_target_incline = cur_incline;
                                treadmill_2ADA_Data[0] = 0x06;
                                treadmill_2ADA_Data[1] = cur_incline & 0xff;
                                treadmill_2ADA_Data[2] = (cur_incline>>8) & 0xff;
                                treadmill_2ADA_send_len = 3;
                                bls_att_pushNotifyData(FITNESS_2ADA_DP_H,treadmill_2ADA_Data,treadmill_2ADA_send_len);
                            }
                        }
                    }

                    u16 cur_degree = (cur_incline * 9)/10;
                    push_buf[7] = cur_incline & 0xff;
                    push_buf[8] = (cur_incline >> 8) & 0xff;
                    push_buf[9] = cur_degree & 0xff;
                    push_buf[10] = (cur_degree >> 8) & 0xff;

                    cur_energy = buf[9] + (buf[10]<<8);
                    if(cur_energy % 10 < 5)
                    {
                        cur_energy = (cur_energy/10) * 10;
                    }else{
                        cur_energy = ((cur_energy + 9)/10) * 10;
                    }
                    cur_energy = cur_energy/10;
                    push_buf[11] = cur_energy & 0xff;
                    push_buf[12] = (cur_energy>>8) & 0xff;
                    push_buf[13] = 0x00;
                    push_buf[14] = 0x00;
                    push_buf[15] = 0x00;
                    push_buf[16] = buf[13];
                    push_buf[17] = buf[5];
                    push_buf[18] = buf[6];

                    push_buf2[2] = cur_speed & 0xff;
                    push_buf2[3] = (cur_speed >> 8) & 0xff;
                    push_buf2[4] = elevation_gain & 0xff;
                    push_buf2[5] = (elevation_gain >> 8) & 0xff;
                    push_buf2[6] = 0;
                    push_buf2[7] = 0;
                    push_buf2[8] = buf[11];
                    push_buf2[9] = buf[12];
                    push_buf2[10] = 0;
                }
                treadmill_2ACD_send_len = 19;
                treadmill_2ACD_send_len2 = 11;
                treadmill_2ACD_send_flag = 3;
                break;
            case 0x02:
                new_target_speed = 100;
                new_target_incline = 0;
                new_target_speed_ticks = clock_time();
                new_target_incline_ticks = clock_time();
                break;
            default:
                treadmill_2ACD_send_len = 19;
                treadmill_2ACD_send_len2 = 11;
                treadmill_2ACD_send_flag = 3;
                break;
        }
    }
    if(len >= 4)
    {
        if(old_status != buf[2])
        {
            switch(buf[2])
            {
                case 0x00:
                case 0x01:
                    if(treadmill_2ADA_Data[0] == 0x04
                            ||treadmill_2ADA_Data[0] == 0x05
                            ||treadmill_2ADA_Data[0] == 0x06
                            ||old_status == 0x0A)
                    {
                        treadmill_2ADA_Data[0] = 0x02;
                        treadmill_2ADA_Data[1] = 0x01;
                        treadmill_2ADA_send_len = 2;
                        treadmill_2ADA_send_flag = 1;
                        treadmill_2ADA_send_ticks = clock_time();
                    }
                    break;
                case 0x02:
                case 0x03:
                    treadmill_2ADA_Data[0] = 0x04;
                    treadmill_2ADA_send_len = 1;
                    treadmill_2ADA_send_flag = 1;
                    treadmill_2ADA_send_ticks = clock_time();
                    break;
                case 0x06:
                    treadmill_2ADA_Data[0] = 0x03;
                    treadmill_2ADA_send_len = 1;
                    treadmill_2ADA_send_flag = 1;
                    treadmill_2ADA_send_ticks = clock_time();
                    break;
                case 0x0A:
                    treadmill_2ADA_Data[0] = 0x02;
                    treadmill_2ADA_Data[1] = 0x02;
                    treadmill_2ADA_send_len = 2;
                    treadmill_2ADA_send_flag = 1;
                    treadmill_2ADA_send_ticks = clock_time();
                    break;
                default:
                    break;
            }
            treadmill_2AD3_Data[0] = 0x01;
            switch(buf[2])
            {
                case 0x01:
                case 0x03:
                case 0x0A:
                    treadmill_2AD3_Data[1] = 0x0D;
                    break;
                case 0x04:
                    treadmill_2AD3_Data[1] = 0x0F;
                    break;
                case 0x02:
                    if(old_status == 0x0A)
                    {
                        treadmill_2AD3_Data[1] = 0x0D;
                    }else{
                        treadmill_2AD3_Data[1] = 0x0E;
                    }
                    break;
                case 0x05:
                case 0x06:
                    treadmill_2AD3_Data[1] = 0x00;
                    break;
                default:
                    treadmill_2AD3_Data[1] = 0x01;
                    // Clear Data
                    treadmill_2ACD_send_len = 19;
                    treadmill_2ACD_send_len2 = 11;
                    treadmill_2ACD_send_flag = 3;
                    break;
            }
            att_set_2AD3_data(treadmill_2AD3_Data,2);
            bls_att_pushNotifyData(FITNESS_2AD3_DP_H,treadmill_2AD3_Data,2);
            old_status = buf[2];
        }
    }
}

void treadmill_respond_speed_range(u8 *buf, u8 len)
{
    if(len >= 7)
    {
        treadmill_2AD4_Data[0] = (buf[4]*10) & 0xff;
        treadmill_2AD4_Data[1] = ((buf[4]*10) >> 8) & 0xff;
        treadmill_2AD4_Data[2] = (buf[3]*10) & 0xff;
        treadmill_2AD4_Data[3] = ((buf[3]*10) >> 8) & 0xff;
        att_set_2AD4_data(treadmill_2AD4_Data,6);
    }
}
void treadmill_respond_incline_range(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        treadmill_2AD5_Data[0] = (buf[4]*10) & 0xff;
        treadmill_2AD5_Data[1] = ((buf[4]*10) >> 8) & 0xff;
        treadmill_2AD5_Data[2] = (buf[3]*10) & 0xff;
        treadmill_2AD5_Data[3] = ((buf[3]*10) >> 8) & 0xff;
        cur_unit = buf[5] & 0x01;
        att_set_2AD5_data(treadmill_2AD5_Data,6);
    }
}

void treadmill_request_status(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x51;
    buf[2] = 0x51;
    buf[3] = 0x03;
    ftms_send(buf,4);
}

void treadmill_request_speed_range(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x50;
    buf[2] = 0x02;
    buf[3] = 0x52;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void treadmill_request_incline_range(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x50;
    buf[2] = 0x03;
    buf[3] = 0x53;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void treadmill_set_target_speed_incline(u8 speed, u8 incline)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x53;
    buf[2] = 0x02;
    buf[3] = speed;
    buf[4] = incline;
    buf[5] = msg_fcs_result(&buf[1],4);
    buf[6] = 0x03;
    ftms_send(buf,7);
}
void treadmill_ctl_start(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x53;
    buf[2] = 0x01;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = 0x00;
    buf[10] = 0x00;
    buf[11] = msg_fcs_result(&buf[1],10);
    buf[12] = 0x03;
    ftms_send(buf,13);
}

void treadmill_ctl_pause(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x53;
    buf[2] = 0x0A;
    buf[3] = msg_fcs_result(&buf[1],2);
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void treadmill_ctl_stop(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x53;
    buf[2] = 0x03;
    buf[3] = msg_fcs_result(&buf[1],2);
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void treadmill_respond(u8 *buf, u8 len)
{
    u16 event = 0;
    if(len >= 4)
    {
        if(len == 4)
        {
            event = buf[1] << 8;
        }else{
            event = (buf[1] << 8) + buf[2];
        }
    }
    switch(event)
    {
        case THINKFIT_50_02:
            treadmill_respond_speed_range(buf,len);
            // Request incline after speed's respond.
            treadmill_request_incline_range();
            break;
        case THINKFIT_50_03:
            treadmill_respond_incline_range(buf,len);
            break;
        case THINKFIT_51_00:
        case THINKFIT_51_01:
        case THINKFIT_51_02:
        case THINKFIT_51_03:
        case THINKFIT_51_04:
        case THINKFIT_51_05:
        case THINKFIT_51_06:
        case THINKFIT_51_09:
        case THINKFIT_51_0A:
            treadmill_respond_status(buf,len);
            break;
    }
}

void treadmill_2AD9_respond(u8 cmd,u8 result)
{
    u8 buf[3];
    buf[0] = 0x80;
    buf[1] = cmd;
    buf[2] = result;
    bls_att_pushNotifyData(FITNESS_2AD9_DP_H,buf,3);
}

void treadmill_2AD9_receive_callback(u8 *buf,u8 len)
{
    u16 target_speed = 0, target_incline = 0;
    if(len > 0)
    {
        switch(buf[0])
        {
            case FTMS_TREADMILL_REQ_CTRL:
                treadmill_2AD9_respond(FTMS_TREADMILL_REQ_CTRL,FTMS_TREADMILL_RSP_SUCCESS);
                break;
            case FTMS_TREADMILL_REQ_RESET:
                treadmill_2AD9_respond(FTMS_TREADMILL_REQ_RESET,FTMS_TREADMILL_RSP_SUCCESS);
                break;
            case FTMS_TREADMILL_SET_SPEED:
                target_speed = buf[1] + (buf[2]<<8);
                treadmill_set_target_speed_incline((u8)(target_speed/10),(u8)(cur_incline/10));
                treadmill_2AD9_respond(FTMS_TREADMILL_SET_SPEED,FTMS_TREADMILL_RSP_SUCCESS);
                break;
            case FTMS_TREADMILL_SET_INCLINE:
                target_incline = buf[1] + (buf[2]<<8);
                treadmill_set_target_speed_incline((u8)(cur_speed/10),(u8)(target_incline/10));
                treadmill_2AD9_respond(FTMS_TREADMILL_SET_INCLINE,FTMS_TREADMILL_RSP_SUCCESS);
                break;
            case FTMS_TREADMILL_CTL_START:
                treadmill_ctl_start();
                treadmill_2AD9_respond(FTMS_TREADMILL_CTL_START,FTMS_TREADMILL_RSP_SUCCESS);
                break;
            case FTMS_TREADMILL_CTL_STOP:
                if(buf[1] == 0x01)
                {
                    treadmill_ctl_stop();
                }
                if(buf[1] == 0x02)
                {
                    treadmill_ctl_pause();
                }
                treadmill_2AD9_respond(FTMS_TREADMILL_CTL_STOP,FTMS_TREADMILL_RSP_SUCCESS);
                break;
            default:
                treadmill_2AD9_respond(buf[0],FTMS_TREADMILL_RSP_NOT_SUPP);
                break;
        }
    }
}

extern void treadmill_att_init(void);
void treadmill_set_type(u8 type)
{
    att_set_2ACC_data(treadmill_2ACC_Data,8);
    treadmill_request_speed_range();
}

void treadmill_proc(void)
{
    if(treadmill_2ACD_send_flag > 0)
    {
#if(SYS_CONFIG_USE_SDK4_EN == 1)
        if(clock_time_exceed(treadmill_2ACD_send_ticks,950000))
#else
        if(clock_time_exceed(treadmill_2ACD_send_ticks,800000))
#endif
        {
            if(BLE_SUCCESS == bls_att_pushNotifyData(TREADMILL_2ACD_DP_H,push_buf,treadmill_2ACD_send_len))
            {
                bls_att_pushNotifyData(TREADMILL_2ACD_DP_H,push_buf2,treadmill_2ACD_send_len2);
            }
            treadmill_2ACD_send_flag--;
            treadmill_2ACD_send_ticks = clock_time();
        }
    }

    if(treadmill_2ADA_send_flag > 0)
    {
        if(clock_time_exceed(treadmill_2ADA_send_ticks,500000))
        {
            treadmill_2ADA_send_flag--;
            treadmill_2ADA_send_ticks = clock_time();
            if(treadmill_2ADA_send_len > 0 && treadmill_2ADA_send_len <= 3)
                bls_att_pushNotifyData(FITNESS_2ADA_DP_H,treadmill_2ADA_Data,treadmill_2ADA_send_len);
        }
    }
}
