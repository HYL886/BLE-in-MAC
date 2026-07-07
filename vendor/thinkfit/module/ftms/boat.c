#include "boat.h"
#include "msg.h"
#include "ble.h"
#include "ftms.h"

enum {
    FTMS_BOAT_RSP_SUCCESS = 0x01,
    FTMS_BOAT_RSP_NOT_SUPP = 0x02,
    FTMS_BOAT_RSP_INV_PARA = 0x03,
    FTMS_BOAT_RSP_FAILED = 0x04,
    FTMS_BOAT_RSP_NOT_PERM = 0x05,
};
enum {
    FTMS_BOAT_REQ_CTRL = 0x00,
    FTMS_BOAT_REQ_RESET = 0x01,
    FTMS_BOAT_SET_SPEED = 0x02,
    FTMS_BOAT_SET_RESISTER = 0x04,
    FTMS_BOAT_SET_POWER = 0x05,
    FTMS_BOAT_CTL_START = 0x07,
    FTMS_BOAT_CTL_STOP = 0x08,
    FTMS_BOAT_SET_SIMULATION = 0x11,
};

extern void att_set_2ACC_data(u8 *buf, u8 len);
extern void att_set_2AD3_data(u8 *buf, u8 len);
extern void att_set_2AD6_data(u8 *buf, u8 len);
extern void att_set_2AD8_data(u8 *buf, u8 len);

static u8 boat_2ACC_Data[8] = {0xA6,0x52,0x00,0x00,0x04,0x00,0x00,0x00};
static u8 boat_2AD3_Data[2] = {0x01,0x01};
static u8 boat_2AD4_Data[6] = {0x00,0x00,0x0F,0x27,0x01,0x00};
static u8 boat_2AD6_Data[6] = {0x0a,0x00,0x0a,0x00,0x0a,0x00};
static u8 boat_2AD8_Data[6] = {0x00,0x00,0xE8,0x03,0x01,0x00};
static u8 boat_2ADA_Data[2] = {0x00,0x00};
static u8 boat_2ADA_Data_len = 0;

static u16 cur_speed = 0,cur_cadence = 0,cur_av_cadence = 0,cur_resister_level = 0;
static u16 cur_counter = 0,cur_power = 0,cur_av_power = 0;
static u16 cur_distance = 0, cur_pace = 0,cur_av_pace = 0;
static u16 cur_time = 0,cur_energy = 0;
static u8 cur_heartrate = 0;
static u8 dev_resister_max = 0;

static u8 push_buf[MSG_MAX_LEN];
static u8 push_buf2[MSG_MAX_LEN];

static u8 boat_2AD1_send_flag = 0;
static u8 boat_2AD1_send_len = 0;
static u8 boat_2AD1_send_len2 = 0;
static u32 boat_2AD1_send_ticks = 0;

static u8 boat_unit_type = 0;

void boat_respond_status(u8 *buf, u8 len)
{
    static u8 old_status = 0;
    static u8 old_resister = 0;
    memset(push_buf,0x00,MSG_MAX_LEN);
    memset(push_buf2,0x00,MSG_MAX_LEN);
    push_buf[0] = 0xFF;
    push_buf[1] = 0x00;
    push_buf2[0] = 0x00;
    push_buf2[1] = 0x0B;
    if(len >= 4)
    {
        switch(buf[2])
        {
            case 2:
                if(old_resister != buf[5])
                {
                    old_resister = buf[5];
                    boat_2ADA_Data[0] = 0x07;
                    boat_2ADA_Data[1] = buf[5] * 10;
                    boat_2ADA_Data_len = 2;
                }
            case 3:
                if(len >= 15)
                {
                    cur_cadence = (buf[6] + (buf[7] << 8))<<1;
                    // push_buf[2] = (cur_cadence) & 0xff;

                    // push_buf[3] = (cur_counter) & 0xff;
                    // push_buf[4] = ((cur_counter) >> 8) & 0xff;
                    if(cur_time > 0)
                    {
                        cur_av_cadence = (cur_counter*60*2)/cur_time;
                    }else{
                        cur_av_cadence = 0;
                    }
                    push_buf[2] = (cur_av_cadence) & 0xff;
                    u32 tmp_distance = cur_distance;
                    if(cur_distance & 0x8000)
                    {
                        tmp_distance = (cur_distance & 0x7fff) * 10;
                    }
                    if(boat_unit_type == 1)
                    {
                        tmp_distance = tmp_distance*100/62;
                    }
                    push_buf[3] = tmp_distance & 0xff;
                    push_buf[4] = (tmp_distance >> 8) & 0xff;
                    push_buf[5] = (tmp_distance >> 16) & 0xff;

                    cur_speed = buf[3] + (buf[4] << 8);
                    if(cur_speed > 0)
                    {
                        if(boat_unit_type == 1)
                        {
                            cur_pace = (3600*100*62/100)/(2*cur_speed);
                        }
                        else
                        {
                            cur_pace = (3600*100)/(2*cur_speed);
                        }
                    }else{
                        cur_pace = 0;
                    }
                    push_buf[6] = (cur_pace) & 0xff;
                    push_buf[7] = ((cur_pace) >> 8) & 0xff;

                    if(tmp_distance > 0)
                    {
                        cur_av_pace = (cur_time * 500)/ tmp_distance;
                    }else{
                        cur_av_pace = 0;
                    }
                    push_buf[8] = (cur_av_pace) & 0xff;
                    push_buf[9] = ((cur_av_pace) >> 8) & 0xff;

                    cur_power = buf[9] + (buf[10] << 8);
                    cur_power = cur_power/10;
                    push_buf[10] = cur_power & 0xff;
                    push_buf[11] = ((cur_power) >> 8) & 0xff;

                    if(cur_time > 0)
                    {
                        cur_av_power = (cur_energy * 4186)/(cur_time*30);
                    }else{
                        cur_av_power = 0;
                    }
                    push_buf[12] = cur_av_power & 0xff;
                    push_buf[13] = ((cur_av_power) >> 8) & 0xff;



                    cur_resister_level = buf[5];
                    push_buf[14] = cur_resister_level & 0xff;
                    push_buf[15] = ((cur_resister_level) >> 8) & 0xff;

                    push_buf2[2] = (cur_cadence) & 0xff;

                    push_buf2[3] = (cur_counter) & 0xff;
                    push_buf2[4] = ((cur_counter) >> 8) & 0xff;

					if(cur_energy % 10 < 5)
                    {
                        cur_energy = (cur_energy/10) * 10;
                    }else{
                        cur_energy = ((cur_energy + 9)/10) * 10;
                    }
                    push_buf2[5] = (cur_energy/10) & 0xff;
                    push_buf2[6] = ((cur_energy/10) >> 8) & 0xff;

                    push_buf2[7] = 0;
                    push_buf2[8] = 0;
                    push_buf2[9] = 0;
					cur_heartrate = buf[8];
                    push_buf2[10] = cur_heartrate;

                    push_buf2[11] = (cur_time) & 0xff;
                    push_buf2[12] = ((cur_time) >> 8) & 0xff;
                }

                boat_2AD1_send_len = 16;
                boat_2AD1_send_len2 = 13;
                boat_2AD1_send_flag = 3;
                break;
            default:
                boat_2AD1_send_len = 16;
                boat_2AD1_send_len2 = 13;
                boat_2AD1_send_flag = 3;
                break;
        }
    }
    if(len >= 4)
    {
        if(old_status != buf[2])
        {
            switch(buf[2])
            {
                case 0:
					boat_2ADA_Data[0] = 0x02;
					boat_2ADA_Data[1] = 0x01;
					boat_2ADA_Data_len = 2;
                    break;
                case 1:
                case 2:
                    boat_2ADA_Data[0] = 0x04;
                    boat_2ADA_Data_len = 1;
                    break;
                case 3:
                    boat_2ADA_Data[0] = 0x02;
                    boat_2ADA_Data[1] = 0x02;
                    boat_2ADA_Data_len = 2;
                    break;
                case 21:
                    boat_2ADA_Data[0] = 0x03;
                    boat_2ADA_Data_len = 1;
                    break;
                default:
                    break;
            }
            boat_2AD3_Data[0] = 0x01;
            switch(buf[2])
            {
                case 0x00:
                    boat_2AD3_Data[1] = 0x01;
                    break;
                case 0x01:
                    boat_2AD3_Data[1] = 0x0E;
                    break;
                case 0x02:
                case 0x03:
                    boat_2AD3_Data[1] = 0x0D;
                    break;
                case 20:
                case 21:
                default:
                    boat_2AD3_Data[1] = 0x00;
                    // Clear Data
                    boat_2AD1_send_len = 16;
                    boat_2AD1_send_len2 = 13;
                    boat_2AD1_send_flag = 3;
                    break;
            }
            att_set_2AD3_data(boat_2AD3_Data,2);
            bls_att_pushNotifyData(FITNESS_2AD3_DP_H,boat_2AD3_Data,2);
            old_status = buf[2];
        }
    }
    if(boat_2ADA_Data_len > 0)
    {
        bls_att_pushNotifyData(FITNESS_2ADA_DP_H,boat_2ADA_Data,boat_2ADA_Data_len);
        boat_2ADA_Data_len = 0;
    }
}
void boat_respond_sport_data(u8 *buf, u8 len)
{
    if(len >= 8)
    {
        cur_time = buf[3] + (buf[4] << 8);
        cur_distance = buf[5] + (buf[6] << 8);
        cur_energy = buf[7] + (buf[8] << 8);
        cur_counter = buf[9] + (buf[10] << 8);
    }
}

void boat_request_sport_data(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x43;
    buf[2] = 0x01;
    buf[3] = 0x42;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void boat_request_status(void)
{
    static u32 counter = 1;
    if(counter++ % 2)
    {
        u8 buf[MSG_MAX_LEN];
        buf[0] = 0x02;
        buf[1] = 0x42;
        buf[2] = 0x42;
        buf[3] = 0x03;
        ftms_send(buf,4);
    }else{
        boat_request_sport_data();
    }
}

void boat_request_device_info(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x41;
    buf[2] = 0x02;
    buf[3] = 0x43;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void boat_respond_device_info(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        boat_2AD6_Data[0] = 0x0a;
        boat_2AD6_Data[1] = 0x00;
        boat_2AD6_Data[2] = (buf[3]*10) & 0xff;
        boat_2AD6_Data[3] = ((buf[3]*10) >> 8) & 0xff;
        dev_resister_max = buf[3];
        att_set_2AD6_data(boat_2AD6_Data,6);

        boat_unit_type  = buf[5]&0x01;
    }
}

void boat_set_user_info(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x0A;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x46;
    buf[8] = 0xAA;
    buf[9] = 0x19;
    buf[10] = 0x00;
    buf[11] = msg_fcs_result(&buf[1],10);
    buf[12] = 0x03;
    ftms_send(buf,13);
}

void boat_set_resister_incline(u8 resister, u8 incline)
{
    if(resister > dev_resister_max || resister < 1)
    {
        return;
    }
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x05;
    buf[3] = resister;
    buf[4] = incline;
    buf[5] = msg_fcs_result(&buf[1],4);
    buf[6] = 0x03;
    ftms_send(buf,7);
}

void boat_set_power(u16 power)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x0B;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x30;
    buf[8] = 0x00;
    buf[9] = power & 0xff;
    buf[10] = (power>>8) & 0xff;
    buf[11] = msg_fcs_result(&buf[1],10);
    buf[12] = 0x03;
    ftms_send(buf,13);
}

void boat_set_boat_simulation(u8 *buf, u8 len)
{
    s16 grade;
    u8 percent = 0;
    grade = *(s16 *)&buf[3];

    if(grade < -20)
    {
        percent = 0x80;
        grade = -grade;
    }else{
        percent = 0;
    }
    if(grade >= -20 && grade <= 20){
        percent = 0;
    }else{
        percent |= (grade/50 + 1);
    }
    boat_set_resister_incline(percent,0);
}

void boat_ctl_start(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x01;
    buf[3] = 0x45;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void boat_ctl_resume(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x02;
    buf[3] = 0x46;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void boat_ctl_pause(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x03;
    buf[3] = 0x47;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void boat_ctl_stop(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x04;
    buf[3] = 0x40;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void boat_respond(u8 *buf, u8 len)
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
        case THINKFIT_41_02:
            boat_respond_device_info(buf,len);
            boat_set_user_info();
            break;
        case THINKFIT_41_04:
            break;

        case THINKFIT_42_00:
        case THINKFIT_42_01:
        case THINKFIT_42_02:
        case THINKFIT_42_03:
        case THINKFIT_42_14:
        case THINKFIT_42_15:
            boat_respond_status(buf,len);
            break;

        case THINKFIT_43_01:
            boat_respond_sport_data(buf,len);
            break;
        case THINKFIT_44_01:
            break;
        case THINKFIT_44_02:
            break;
        case THINKFIT_44_03:
            break;
        case THINKFIT_44_04:
            break;
        case THINKFIT_44_05:
            break;
        case THINKFIT_44_0A:
            break;
    }
}

void boat_2AD9_respond(u8 cmd, u8 result)
{
    u8 buf[3];
    buf[0] = 0x80;
    buf[1] = cmd;
    buf[2] = result;
    bls_att_pushNotifyData(FITNESS_2AD9_DP_H,buf,3);
}

void boat_2AD9_receive_callback(u8 *buf,u8 len)
{
    u16 target_power = 0;
    u16 target_resister_level = 0;
    if(len > 0)
    {
        switch(buf[0])
        {
            case FTMS_BOAT_REQ_CTRL:
                boat_2AD9_respond(FTMS_BOAT_REQ_CTRL,FTMS_BOAT_RSP_SUCCESS);
                break;
            case FTMS_BOAT_REQ_RESET:
                boat_ctl_stop();
                boat_2AD9_respond(FTMS_BOAT_REQ_RESET,FTMS_BOAT_RSP_SUCCESS);
                break;
            case FTMS_BOAT_SET_SPEED:
                boat_2AD9_respond(FTMS_BOAT_SET_SPEED,FTMS_BOAT_RSP_SUCCESS);
                break;
            case FTMS_BOAT_SET_RESISTER:
				if(len == 3)
                {
                    target_resister_level = buf[1] + (buf[2] << 8);
                }
                if(len == 2)
                {
                    target_resister_level = buf[1];
                }
                if((target_resister_level >= 10) && (target_resister_level <= (dev_resister_max*10)))
                {
					boat_set_resister_incline(target_resister_level/10,0);
					boat_2AD9_respond(FTMS_BOAT_SET_RESISTER,FTMS_BOAT_RSP_SUCCESS);
				}else{
                    boat_2AD9_respond(FTMS_BOAT_SET_RESISTER,FTMS_BOAT_RSP_FAILED);
                }
                
                break;
            case FTMS_BOAT_SET_POWER:
                target_power = buf[1] + (buf[2]<<8);
                boat_set_power(target_power);
                boat_2AD9_respond(FTMS_BOAT_SET_POWER,FTMS_BOAT_RSP_SUCCESS);
                break;
            case FTMS_BOAT_CTL_START:
                if((boat_2ADA_Data[0] == 0x02) && (boat_2ADA_Data[1] == 0x02))
                {
                    boat_ctl_resume();
                }
                if(boat_2AD3_Data[1] == 0x01)
                {
                    boat_ctl_start();
					boat_ctl_resume();
                }
                boat_2AD9_respond(FTMS_BOAT_CTL_START,FTMS_BOAT_RSP_SUCCESS);
                break;
            case FTMS_BOAT_CTL_STOP:
                if(len > 1)
                {
                    if(buf[1] == 0x01)
                        boat_ctl_stop();
                    if(buf[1] == 0x02)
                        boat_ctl_pause();
                    boat_2AD9_respond(FTMS_BOAT_CTL_STOP,FTMS_BOAT_RSP_SUCCESS);
                }
                break;
            case FTMS_BOAT_SET_SIMULATION:
                boat_set_boat_simulation(buf,len);
                boat_2AD9_respond(FTMS_BOAT_SET_SIMULATION,FTMS_BOAT_RSP_SUCCESS);
                break;
            default:
                boat_2AD9_respond(buf[0],FTMS_BOAT_RSP_NOT_SUPP);
                break;
        }
    }
}

void boat_set_type(u8 type)
{
	boat_2AD6_Data[2] = (dev_resister_max*10) & 0xff;
    boat_2AD6_Data[3] = ((dev_resister_max*10) >> 8) & 0xff;
    att_set_2ACC_data(boat_2ACC_Data,8);
	att_set_2AD4_data(boat_2AD4_Data,6);
    att_set_2AD6_data(boat_2AD6_Data,6);
    att_set_2AD8_data(boat_2AD8_Data,6);
    boat_request_device_info();
}

void boat_proc(void)
{
    if(boat_2AD1_send_flag > 0)
    {
        if(clock_time_exceed(boat_2AD1_send_ticks,800000))
        {
            if(BLE_SUCCESS == bls_att_pushNotifyData(BOAT_2AD1_DP_H,push_buf,boat_2AD1_send_len))
            {
                bls_att_pushNotifyData(BOAT_2AD1_DP_H,push_buf2,boat_2AD1_send_len2);
            }
            boat_2AD1_send_flag--;
            boat_2AD1_send_ticks = clock_time();
        }
    }
}
