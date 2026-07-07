#include "crtr.h"
#include "msg.h"
#include "ble.h"
#include "ftms.h"

enum {
    FTMS_CRTR_RSP_SUCCESS = 0x01,
    FTMS_CRTR_RSP_NOT_SUPP = 0x02,
    FTMS_CRTR_RSP_INV_PARA = 0x03,
    FTMS_CRTR_RSP_FAILED = 0x04,
    FTMS_CRTR_RSP_NOT_PERM = 0x05,
};
enum {
    FTMS_CRTR_REQ_CTRL = 0x00,
    FTMS_CRTR_REQ_RESET = 0x01,
    FTMS_CRTR_SET_SPEED = 0x02,
    FTMS_CRTR_SET_RESISTER = 0x04,
    FTMS_CRTR_SET_POWER = 0x05,
    FTMS_CRTR_CTL_START = 0x07,
    FTMS_CRTR_CTL_STOP = 0x08,
    FTMS_CRTR_SET_SIMULATION = 0x11,
};

extern void att_set_2ACC_data(u8 *buf, u8 len);
extern void att_set_2AD3_data(u8 *buf, u8 len);
extern void att_set_2AD6_data(u8 *buf, u8 len);
extern void att_set_2AD8_data(u8 *buf, u8 len);

static u8 crtr_2ACC_Data[8] = {0xC7,0x52,0x00,0x00,0x04,0x00,0x00,0x00};
static u8 crtr_2AD3_Data[2] = {0x01,0x01};
static u8 crtr_2AD4_Data[6] = {0x00,0x00,0x0F,0x27,0x01,0x00};
static u8 crtr_2AD6_Data[6] = {0x0a,0x00,0x0a,0x00,0x0a,0x00};
static u8 crtr_2AD8_Data[6] = {0x00,0x00,0xE8,0x03,0x01,0x00};
static u8 crtr_2ADA_Data[3] = {0x00,0x00,0x00};
static u8 crtr_2ADA_Data_len = 0;

static u16 cur_speed = 0,cur_av_speed = 0,cur_cadence = 0;
static u16 cur_counter = 0,cur_stride_count = 0,cur_av_cadence = 0;
static u16 cur_time = 0,cur_energy = 0,cur_resister_level = 0;
static u16 cur_distance = 0,cur_power = 0,cur_av_power = 0;
static u8 cur_heartrate = 0;
static u8 dev_resister_max = 0;

static u8 push_buf[MSG_MAX_LEN];
static u8 push_buf2[MSG_MAX_LEN];

static u8 crtr_2ACE_send_flag = 0;
static u8 crtr_2ACE_send_len = 0;
static u8 crtr_2ACE_send_len2 = 0;
static u32 crtr_2ACE_send_ticks = 0;

static u8 crtr_unit_type = 0;

void crtr_respond_status(u8 *buf, u8 len)
{
    static u8 old_status = 0;
    static u8 old_resister = 0;
    memset(push_buf,0x00,MSG_MAX_LEN);
    memset(push_buf2,0x00,MSG_MAX_LEN);
    push_buf[0] = 0x9F;
    push_buf[1] = 0x00;
    push_buf2[0] = 0x00;
    push_buf2[1] = 0x2F;
    if(len >= 4)
    {
        switch(buf[2])
        {
            case 2:
                if(old_resister != buf[5])
                {
                    old_resister = buf[5];
                    crtr_2ADA_Data[0] = 0x07;
                    crtr_2ADA_Data[1] = (buf[5] * 10) & 0xff;
                    crtr_2ADA_Data[2] = ((buf[5] * 10) >> 8) & 0xff;
                    crtr_2ADA_Data_len = 3;
                }
            case 3:
                if(len >= 15)
                {
                    push_buf[2] = 0;
                    cur_speed = buf[3] + (buf[4] << 8);
                    if(crtr_unit_type == 1)
                    {
                        cur_speed = cur_speed*100/62;
                    }
                    //push_buf[3] = (cur_speed) & 0xff;
                    //push_buf[4] = (cur_speed >> 8) & 0xff;
                    u32 tmp_distance = cur_distance;
                    if(cur_distance & 0x8000)
                    {
                        tmp_distance = (cur_distance & 0x7fff) * 10;
                    }
                    if(crtr_unit_type == 1)
                    {
                        tmp_distance = tmp_distance*100/62;
                    }

                    if(cur_time > 0)
                    {
                        cur_av_speed = (tmp_distance * 3600)/(cur_time*10);
                    }else{
                        cur_av_speed = 0;
                    }
                    push_buf[3] = (cur_av_speed) & 0xff;
                    push_buf[4] = (cur_av_speed >> 8) & 0xff;
                    push_buf[5] = tmp_distance & 0xff;
                    push_buf[6] = (tmp_distance >> 8) & 0xff;
                    push_buf[7] = (tmp_distance >> 16) & 0xff;

                    cur_cadence = buf[6] + (buf[7] << 8);
                    push_buf[8] = (cur_cadence) & 0xff;
                    push_buf[9] = (cur_cadence >> 8) & 0xff;

                    if(cur_time > 0)
                    {
                        cur_av_cadence = (cur_counter*60)/cur_time;
                    }else{
                        cur_av_cadence = 0;
                    }
                    push_buf[10] = (cur_av_cadence) & 0xff;
                    push_buf[11] = (cur_av_cadence >> 8) & 0xff;

                    cur_stride_count = (cur_counter/2)*10;
                    push_buf[12] = (cur_stride_count) & 0xff;
                    push_buf[13] = (cur_stride_count >> 8) & 0xff;

                    cur_resister_level = buf[5]*10;
                    push_buf[14] = (cur_resister_level) & 0xff;
                    push_buf[15] = (cur_resister_level >> 8) & 0xff;

                    push_buf2[2] = 0;
                    push_buf2[3] = (cur_speed) & 0xff;
                    push_buf2[4] = (cur_speed >> 8) & 0xff;

                    cur_power = buf[9] + (buf[10] << 8);
                    cur_power = cur_power/10;
                    push_buf2[5] = cur_power & 0xff;
                    push_buf2[6] = ((cur_power) >> 8) & 0xff;

                    if(cur_time > 0)
                    {
                        cur_av_power = (cur_energy * 4186)/(cur_time*50);
                    }else{
                        cur_av_power = 0;
                    }
                    if(cur_energy % 10 < 5)
                    {
                        cur_energy = (cur_energy/10) * 10;
                    }else{
                        cur_energy = ((cur_energy + 9)/10) * 10;
                    }

                    push_buf2[7] = cur_av_power & 0xff;
                    push_buf2[8] = ((cur_av_power) >> 8) & 0xff;

                    push_buf2[9] = (cur_energy/10) & 0xff;
                    push_buf2[10] = ((cur_energy/10) >> 8) & 0xff;
                    push_buf2[11] = 0;
                    push_buf2[12] = 0;
                    push_buf2[13] = 0;
                    cur_heartrate = buf[8];
                    push_buf2[14] = cur_heartrate;
                    push_buf2[15] = (cur_time) & 0xff;
                    push_buf2[16] = ((cur_time) >> 8) & 0xff;
                }

                crtr_2ACE_send_len = 16;
                crtr_2ACE_send_len2 = 17;
                crtr_2ACE_send_flag = 3;
                break;
            default:
                crtr_2ACE_send_len = 16;
                crtr_2ACE_send_len2 = 17;
                crtr_2ACE_send_flag = 3;
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
					crtr_2ADA_Data[0] = 0x02;
					crtr_2ADA_Data[1] = 0x01;
					crtr_2ADA_Data_len = 2;
                    break;
                case 1:
                case 2:
                    crtr_2ADA_Data[0] = 0x04;
                    crtr_2ADA_Data_len = 1;
                    break;
                case 3:
                    crtr_2ADA_Data[0] = 0x02;
                    crtr_2ADA_Data[1] = 0x02;
                    crtr_2ADA_Data_len = 2;
                    break;
                case 21:
                    crtr_2ADA_Data[0] = 0x03;
                    crtr_2ADA_Data_len = 1;
                    break;
                default:
                    break;
            }
            crtr_2AD3_Data[0] = 0x01;
            switch(buf[2])
            {
                case 0x00:
                    crtr_2AD3_Data[1] = 0x01;
                    break;
                case 0x01:
                    crtr_2AD3_Data[1] = 0x0E;
                    break;
                case 0x02:
                case 0x03:
                    crtr_2AD3_Data[1] = 0x0D;
                    break;
                case 20:
                case 21:
                default:
                    crtr_2AD3_Data[1] = 0x00;
                    // Clear Data
                    crtr_2ACE_send_len = 16;
                    crtr_2ACE_send_len2 = 17;
                    crtr_2ACE_send_flag = 3;
                    break;
            }
            att_set_2AD3_data(crtr_2AD3_Data,2);
            bls_att_pushNotifyData(FITNESS_2AD3_DP_H,crtr_2AD3_Data,2);
            old_status = buf[2];
        }
    }
    if(crtr_2ADA_Data_len > 0)
    {
        bls_att_pushNotifyData(FITNESS_2ADA_DP_H,crtr_2ADA_Data,crtr_2ADA_Data_len);
        crtr_2ADA_Data_len = 0;
    }
}
void crtr_respond_sport_data(u8 *buf, u8 len)
{
    if(len >= 8)
    {
        cur_time = buf[3] + (buf[4] << 8);
        cur_distance = buf[5] + (buf[6] << 8);
        cur_energy = buf[7] + (buf[8] << 8);
        cur_counter = buf[9] + (buf[10] << 8);
    }
}

void crtr_request_sport_data(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x43;
    buf[2] = 0x01;
    buf[3] = 0x42;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void crtr_request_status(void)
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
        crtr_request_sport_data();
    }
}

void crtr_request_device_info(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x41;
    buf[2] = 0x02;
    buf[3] = 0x43;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void crtr_respond_device_info(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        crtr_2AD6_Data[0] = 0x0a;
        crtr_2AD6_Data[1] = 0x00;
		dev_resister_max = buf[3];
        crtr_2AD6_Data[2] = (dev_resister_max*10) & 0xff;
        crtr_2AD6_Data[3] = ((dev_resister_max*10) >> 8) & 0xff;
        crtr_2AD6_Data[2] = (buf[3]*10) & 0xff;
        crtr_2AD6_Data[3] = ((buf[3]*10) >> 8) & 0xff;
        att_set_2AD6_data(crtr_2AD6_Data,6);

        crtr_unit_type  = buf[5]&0x01;
    }
}

void crtr_set_user_info(void)
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

void crtr_set_resister_incline(u8 resister, u8 incline)
{
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

void crtr_set_power(u16 power)
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

void crtr_set_crtr_resister_level(u8 *buf, u8 len)
{
    u8 *grade;
    u8 percent = 0;
    grade = &buf[1];
    if(*grade <= 20)
    {
        percent = 0;
    }else{
        percent = (*grade-20)/6 + 1;
        if(percent > 32)
        {
            percent = 32;
        }
    }
    crtr_set_resister_incline(percent,0);
}

void crtr_set_crtr_simulation(u8 *buf, u8 len)
{
    s16 grade;
    u16 factory;
    grade = *(s16 *)&buf[3];
    factory = *(u16 *)&buf[5];
    //printf("get %d\n",grade);
#if (SYS_USE_BEEP == 1)
	ckit_bike_cmd_from_silent();
#endif
    switch(factory)
    {
        case 0x3328:
			if(grade < 0)
		    {
		        grade = 0;    
		    }
            crtr_set_resister_incline(grade/25 + 1,0);
            break;

		case 0x2221:
			if(grade < 0)
		    {
		        grade = 0;    
		    }
            crtr_set_resister_incline(grade*(dev_resister_max-1)/1600 + 1,0);
            break;

        default:
			grade += 850; 
            crtr_set_resister_incline(grade*(dev_resister_max-1)/2350 + 1,0);
    }
}

void crtr_ctl_start(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x01;
    buf[3] = 0x45;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void crtr_ctl_resume(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x02;
    buf[3] = 0x46;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void crtr_ctl_pause(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x03;
    buf[3] = 0x47;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void crtr_ctl_stop(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x04;
    buf[3] = 0x40;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void crtr_respond(u8 *buf, u8 len)
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
            crtr_respond_device_info(buf,len);
            crtr_set_user_info();
            break;
        case THINKFIT_41_04:
            break;

        case THINKFIT_42_00:
        case THINKFIT_42_01:
        case THINKFIT_42_02:
        case THINKFIT_42_03:
        case THINKFIT_42_14:
        case THINKFIT_42_15:
            crtr_respond_status(buf,len);
            break;

        case THINKFIT_43_01:
            crtr_respond_sport_data(buf,len);
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

void crtr_2AD9_respond(u8 cmd, u8 result)
{
    u8 buf[3];
    buf[0] = 0x80;
    buf[1] = cmd;
    buf[2] = result;
    bls_att_pushNotifyData(FITNESS_2AD9_DP_H,buf,3);
}

void crtr_2AD9_receive_callback(u8 *buf,u8 len)
{
    u16 target_power = 0;
    u16 target_resister_level = 0;
	
    if(len > 0)
    {
        switch(buf[0])
        {
            case FTMS_CRTR_REQ_CTRL:
                crtr_2AD9_respond(FTMS_CRTR_REQ_CTRL,FTMS_CRTR_RSP_SUCCESS);
                break;
            case FTMS_CRTR_REQ_RESET:
                crtr_2AD9_respond(FTMS_CRTR_REQ_RESET,FTMS_CRTR_RSP_SUCCESS);
                break;
            case FTMS_CRTR_SET_SPEED:
                crtr_2AD9_respond(FTMS_CRTR_SET_SPEED,FTMS_CRTR_RSP_SUCCESS);
                break;
            case FTMS_CRTR_SET_RESISTER:
                //crtr_set_crtr_resister_level(buf,len);
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
                    crtr_set_resister_incline(target_resister_level/10,0);
                    crtr_2AD9_respond(FTMS_CRTR_SET_RESISTER,FTMS_CRTR_RSP_SUCCESS);
                }else{
                    crtr_2AD9_respond(FTMS_CRTR_SET_RESISTER,FTMS_CRTR_RSP_FAILED);
                }
                break;
            case FTMS_CRTR_SET_POWER:
                target_power = buf[1] + (buf[2]<<8);
                crtr_set_power(target_power);
                crtr_2AD9_respond(FTMS_CRTR_SET_POWER,FTMS_CRTR_RSP_SUCCESS);
                break;
            case FTMS_CRTR_CTL_START:
                if((crtr_2ADA_Data[0] == 0x02) && (crtr_2ADA_Data[1] == 0x02))
                {
                    crtr_ctl_resume();
                }
                if(crtr_2AD3_Data[1] == 0x01)
                {
                    crtr_ctl_start();
					crtr_ctl_resume();
                }
                crtr_2AD9_respond(FTMS_CRTR_CTL_START,FTMS_CRTR_RSP_SUCCESS);
                break;
            case FTMS_CRTR_CTL_STOP:
                if(len > 1)
                {
                    if(buf[1] == 0x01)
                        crtr_ctl_stop();
                    if(buf[1] == 0x02)
                        crtr_ctl_pause();
                    crtr_2AD9_respond(FTMS_CRTR_CTL_STOP,FTMS_CRTR_RSP_SUCCESS);
                }
                break;
            case FTMS_CRTR_SET_SIMULATION:
                crtr_set_crtr_simulation(buf,len);
                crtr_2AD9_respond(FTMS_CRTR_SET_SIMULATION,FTMS_CRTR_RSP_SUCCESS);
                break;
            default:
                crtr_2AD9_respond(buf[0],FTMS_CRTR_RSP_NOT_SUPP);
                break;
        }
    }
}

void crtr_set_type(u8 type)
{
	crtr_2AD6_Data[2] = (dev_resister_max*10) & 0xff;
    crtr_2AD6_Data[3] = ((dev_resister_max*10) >> 8) & 0xff;
    att_set_2ACC_data(crtr_2ACC_Data,8);
	att_set_2AD4_data(crtr_2AD4_Data,6);
    att_set_2AD6_data(crtr_2AD6_Data,6);
    att_set_2AD8_data(crtr_2AD8_Data,6);
    crtr_request_device_info();
}

void crtr_proc(void)
{
    if(crtr_2ACE_send_flag > 0)
    {
        if(clock_time_exceed(crtr_2ACE_send_ticks,800000))
        {
            if(BLE_SUCCESS == bls_att_pushNotifyData(CRTR_2ACE_DP_H,push_buf,crtr_2ACE_send_len))
            {
                bls_att_pushNotifyData(CRTR_2ACE_DP_H,push_buf2,crtr_2ACE_send_len2);
            }
            crtr_2ACE_send_flag--;
            crtr_2ACE_send_ticks = clock_time();
        }
    }
}
