#include "bike.h"
#include "msg.h"
#include "ble.h"
#include "ftms.h"

enum {
    FTMS_BIKE_RSP_SUCCESS = 0x01,
    FTMS_BIKE_RSP_NOT_SUPP = 0x02,
    FTMS_BIKE_RSP_INV_PARA = 0x03,
    FTMS_BIKE_RSP_FAILED = 0x04,
    FTMS_BIKE_RSP_NOT_PERM = 0x05,
};
enum {
    FTMS_BIKE_REQ_CTRL = 0x00,
    FTMS_BIKE_REQ_RESET = 0x01,
    FTMS_BIKE_SET_SPEED = 0x02,
    FTMS_BIKE_SET_INCLINATION = 0x03,
    FTMS_BIKE_SET_RESISTER = 0x04,
    FTMS_BIKE_SET_POWER = 0x05,
    FTMS_BIKE_CTL_START = 0x07,
    FTMS_BIKE_CTL_STOP = 0x08,
    FTMS_BIKE_SET_SIMULATION = 0x11,
};

extern void att_set_2ACC_data(u8 *buf, u8 len);
extern void att_set_2AD3_data(u8 *buf, u8 len);
extern void att_set_2AD6_data(u8 *buf, u8 len);

static u8 bike_2ACC_Data[8] = {0x87,0x52,0x00,0x00,0x04,0x00,0x00,0x00};
static u8 bike_2AD3_Data[2] = {0x01,0x01};
static u8 bike_2AD4_Data[6] = {0x00,0x00,0x0F,0x27,0x01,0x00};
static u8 bike_2AD5_Data[6] = {0x00,0x00,0x0a,0x00,0x0a,0x00};
static u8 bike_2AD6_Data[6] = {0x0a,0x00,0x0a,0x00,0x0a,0x00};
static u8 bike_2AD8_Data[6] = {0x00,0x00,0xE8,0x03,0x01,0x00};
static u8 bike_2ADA_Data[2] = {0x00,0x00};
static u8 bike_2ADA_send_len = 0;

static u16 cur_speed = 0,cur_cadence = 0,cur_power = 0;
static u16 cur_distance = 0,cur_time = 0,cur_energy = 0;
static u8 cur_resister = 10,cur_inclination = 0;
static u8 cur_heartrate = 0;
static u8 dev_resister_max = 0;
static u8 dev_inclination_max = 0;

static u8 push_buf[MSG_MAX_LEN];
static u8 push_buf2[MSG_MAX_LEN];

static u8 bike_2AD2_send_flag = 0;
static u8 bike_2AD2_send_len = 0;
static u8 bike_2AD2_send_len2 = 0;
static u32 bike_2AD2_send_ticks = 0;

static u8 bike_unit_type = 0;

void bike_respond_status(u8 *buf, u8 len)
{
    static u8 old_status = 0;
    static u8 old_resister = 0,diff_resister = 0;
    memset(push_buf,0x00,MSG_MAX_LEN);
    memset(push_buf2,0x00,MSG_MAX_LEN);
    push_buf[0] = 0x75;
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
                    if(old_resister > buf[5])
                    {
                        diff_resister = old_resister - buf[5];
                    }else{
                        diff_resister = buf[5] - old_resister;
                    }
                    if(diff_resister > 0 && diff_resister < 25)
                    {
                        bike_2ADA_Data[0] = 0x07;
                        bike_2ADA_Data[1] = diff_resister * 10;
                        bike_2ADA_send_len = 2;
                    }
                    old_resister = buf[5];
                }
            case 3:
                if(len >= 15)
                {
                    cur_cadence = (buf[6] + (buf[7] << 8))<<1;
                    push_buf[2] = (cur_cadence) & 0xff;
                    push_buf[3] = ((cur_cadence) >> 8) & 0xff;
                    u32 tmp_distance = cur_distance;
                    if(cur_distance & 0x8000)
                    {
                        tmp_distance = (cur_distance & 0x7fff) * 10;
                    }
                    if(bike_unit_type == 1)
                    {
                        tmp_distance = tmp_distance*100/62;
                    }

                    push_buf[4] = tmp_distance & 0xff;
                    push_buf[5] = (tmp_distance >> 8) & 0xff;
                    push_buf[6] = (tmp_distance >> 16) & 0xff;

                    push_buf[7] = buf[5];
                    push_buf[8] = 0x00;

                    cur_power = buf[9] + (buf[10] << 8);
                    cur_power = cur_power/10;
                    push_buf[9] = cur_power & 0xff;
                    push_buf[10] = ((cur_power) >> 8) & 0xff;
                    
                    cur_speed = buf[3] + (buf[4] << 8);
                    if(bike_unit_type == 1)
                    {
                        cur_speed = cur_speed*100/62;
                    }
                    push_buf2[2] = cur_speed & 0xff;
                    push_buf2[3] = ((cur_speed) >> 8) & 0xff;

                    cur_energy = cur_energy/10;
                    push_buf2[4] = cur_energy & 0xff;
                    push_buf2[5] = ((cur_energy) >> 8) & 0xff;
                    push_buf2[6] = 0x00;
                    push_buf2[7] = 0x00;
                    push_buf2[8] = 0x00;

                    cur_heartrate = buf[8];
                    push_buf2[9] = cur_heartrate;
                    
                    push_buf2[10] = cur_time & 0xff;
                    push_buf2[11] = ((cur_time) >> 8) & 0xff;

                    
                }
                bike_2AD2_send_len = 11;
                bike_2AD2_send_len2 = 12;
                bike_2AD2_send_flag = 3;
                break;
            default:
                bike_2AD2_send_len = 11;
                bike_2AD2_send_len2 = 12;
                bike_2AD2_send_flag = 3;
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
					bike_2ADA_Data[0] = 0x02;
					bike_2ADA_Data[1] = 0x01;
					bike_2ADA_send_len = 2;
                    break;
                case 0x01:
                case 0x02:
                    bike_2ADA_Data[0] = 0x04;
                    bike_2ADA_send_len = 1;
                    break;
                case 0x03:
                    bike_2ADA_Data[0] = 0x02;
                    bike_2ADA_Data[1] = 0x02;
                    bike_2ADA_send_len = 2;
                    break;
                case 0x15:
                    bike_2ADA_Data[0] = 0x03;
                    bike_2ADA_send_len = 1;
                    break;
                default:
                    break;
            }
            bike_2AD3_Data[0] = 0x01;
            switch(buf[2])
            {
                case 0x00:
                    bike_2AD3_Data[1] = 0x01;
                    break;
                case 0x01:
                    bike_2AD3_Data[1] = 0x0E;
                    break;
                case 0x02:
                case 0x03:
                    bike_2AD3_Data[1] = 0x0D;
                    break;
                case 0x14:
                case 0x15:
                default:
                    bike_2AD3_Data[1] = 0x00;
                    break;
            }
            att_set_2AD3_data(bike_2AD3_Data,2);
            bls_att_pushNotifyData(FITNESS_2AD3_DP_H,bike_2AD3_Data,2);
            old_status = buf[2];
        }
    }
    if(bike_2ADA_send_len > 0)
    {
        bls_att_pushNotifyData(FITNESS_2ADA_DP_H,bike_2ADA_Data,bike_2ADA_send_len);
        bike_2ADA_send_len = 0;
    }
}
void bike_respond_sport_data(u8 *buf, u8 len)
{
    if(len >= 8)
    {
        cur_time = buf[3] + (buf[4] << 8);
        cur_distance = buf[5] + (buf[6] << 8);
        cur_energy = buf[7] + (buf[8] << 8);
        if(cur_energy % 10 < 5)
        {
            cur_energy = (cur_energy/10) * 10;
        }else{
            cur_energy = ((cur_energy + 9)/10) * 10;
        }
    }
}

void bike_request_sport_data(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x43;
    buf[2] = 0x01;
    buf[3] = 0x42;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void bike_request_status(void)
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
        bike_request_sport_data();
    }
}

void bike_request_device_info(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x41;
    buf[2] = 0x02;
    buf[3] = 0x43;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void bike_respond_device_info(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        bike_2AD6_Data[0] = 0x0a;
        bike_2AD6_Data[1] = 0x00;
        dev_resister_max = buf[3];
        bike_2AD6_Data[2] = (dev_resister_max*10) & 0xff;
        bike_2AD6_Data[3] = ((dev_resister_max*10) >> 8) & 0xff;
        att_set_2AD6_data(bike_2AD6_Data,6);
		
		bike_2AD5_Data[0] = 0x00;
        bike_2AD5_Data[1] = 0x00;
        dev_inclination_max = buf[4];
        bike_2AD5_Data[2] = (dev_inclination_max*10) & 0xff;
        bike_2AD5_Data[3] = ((dev_inclination_max*10) >> 8) & 0xff;
        att_set_2AD5_data(bike_2AD5_Data,6);

        bike_unit_type  = buf[5]&0x01;
    }
}

void bike_set_user_info(void)
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

void bike_set_resister_incline(u8 resister, u8 incline)
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

void bike_set_power(u16 power)
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

void bike_set_bike_resister_level(u8 *buf, u8 len)
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
    bike_set_resister_incline(percent,cur_inclination);
}

void bike_set_bike_simulation(u8 *buf, u8 len)
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
            bike_set_resister_incline(grade/25 + 1,0);
            break;

		case 0x2221:
			if(grade < 0)
		    {
		        grade = 0;    
		    }
            bike_set_resister_incline(grade*(dev_resister_max-1)/1600 + 1,0);
            break;

        default:
			grade += 850; 
            bike_set_resister_incline(grade*(dev_resister_max-1)/2350 + 1,0);
    }
}

void bike_ctl_start(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x01;
    buf[3] = 0x45;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void bike_ctl_resume(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x02;
    buf[3] = 0x46;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void bike_ctl_pause(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x03;
    buf[3] = 0x47;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void bike_ctl_stop(void)
{
    u8 buf[MSG_MAX_LEN];
    buf[0] = 0x02;
    buf[1] = 0x44;
    buf[2] = 0x04;
    buf[3] = 0x40;
    buf[4] = 0x03;
    ftms_send(buf,5);
}

void bike_respond(u8 *buf, u8 len)
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
            bike_respond_device_info(buf,len);
            bike_set_user_info();
            break;
        case THINKFIT_41_04:
            break;

        case THINKFIT_42_00:
        case THINKFIT_42_01:
        case THINKFIT_42_02:
        case THINKFIT_42_03:
        case THINKFIT_42_14:
        case THINKFIT_42_15:
            bike_respond_status(buf,len);
            break;

        case THINKFIT_43_01:
            bike_respond_sport_data(buf,len);
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
        case THINKFIT_44_0B:
            break;
    }
}

void bike_2AD9_respond(u8 cmd, u8 result)
{
    u8 buf[3];
    buf[0] = 0x80;
    buf[1] = cmd;
    buf[2] = result;
    bls_att_pushNotifyData(FITNESS_2AD9_DP_H,buf,3);
}

void bike_2AD9_receive_callback(u8 *buf,u8 len)
{
    u16 target_power = 0;
    u16 target_resister_level = 0;
    if(len > 0)
    {
        switch(buf[0])
        {
            case FTMS_BIKE_REQ_CTRL:
                bike_2AD9_respond(FTMS_BIKE_REQ_CTRL,FTMS_BIKE_RSP_SUCCESS);
                break;
            case FTMS_BIKE_REQ_RESET:
                bike_2AD9_respond(FTMS_BIKE_REQ_RESET,FTMS_BIKE_RSP_SUCCESS);
                break;
            case FTMS_BIKE_SET_SPEED:
                bike_2AD9_respond(FTMS_BIKE_SET_SPEED,FTMS_BIKE_RSP_SUCCESS);
                break;
            case FTMS_BIKE_SET_RESISTER:
                if(len == 3)
                {
                    target_resister_level = buf[1] + (buf[2]<<8);
                }
                if(len == 2)
                {
                    target_resister_level = buf[1];
                }
                if((target_resister_level >= 10) && (target_resister_level <= (dev_resister_max*10)))
                {
                    cur_resister = target_resister_level/10;
                    bike_set_resister_incline(cur_resister,cur_inclination);
                    bike_2AD9_respond(FTMS_BIKE_SET_RESISTER,FTMS_BIKE_RSP_SUCCESS);
                }else{
                    bike_2AD9_respond(FTMS_BIKE_SET_RESISTER,FTMS_BIKE_RSP_FAILED);
                }
                break;
            case FTMS_BIKE_SET_INCLINATION:
                cur_inclination = (buf[1]+(buf[2]<<8))/10;
                bike_set_resister_incline(cur_resister,cur_inclination);
                bike_2AD9_respond(FTMS_BIKE_SET_INCLINATION,FTMS_BIKE_RSP_SUCCESS);
                break;
            case FTMS_BIKE_SET_POWER:
                target_power = buf[1] + (buf[2]<<8);
                bike_set_power(target_power);
                bike_2AD9_respond(FTMS_BIKE_SET_POWER,FTMS_BIKE_RSP_SUCCESS);
                break;
            case FTMS_BIKE_CTL_START:
                if((bike_2ADA_Data[0] == 0x02) && (bike_2ADA_Data[1] == 0x02))
                {
                    bike_ctl_resume();
                }
                if(bike_2AD3_Data[1] == 0x01)
                {
                    bike_ctl_start();
					bike_ctl_resume();
                }
                bike_2AD9_respond(FTMS_BIKE_CTL_START,FTMS_BIKE_RSP_SUCCESS);
                break;
            case FTMS_BIKE_CTL_STOP:
                if(len > 1)
                {
                    if(buf[1] == 0x01)
                        bike_ctl_stop();
                    if(buf[1] == 0x02)
                        bike_ctl_pause();
                    bike_2AD9_respond(FTMS_BIKE_CTL_STOP,FTMS_BIKE_RSP_SUCCESS);
                }
                break;
            case FTMS_BIKE_SET_SIMULATION:
                bike_set_bike_simulation(buf,len);
                bike_2AD9_respond(FTMS_BIKE_SET_SIMULATION,FTMS_BIKE_RSP_SUCCESS);
                break;
            default:
                bike_2AD9_respond(buf[0],FTMS_BIKE_RSP_NOT_SUPP);
                break;
        }
    }
}

void bike_set_type(u8 type)
{
	bike_2AD6_Data[2] = (dev_resister_max*10) & 0xff;
    bike_2AD6_Data[3] = ((dev_resister_max*10) >> 8) & 0xff;
    att_set_2ACC_data(bike_2ACC_Data,8);
	att_set_2AD4_data(bike_2AD4_Data,6);
    att_set_2AD6_data(bike_2AD6_Data,6);
    att_set_2AD8_data(bike_2AD8_Data,6);
    bike_request_device_info();
}

void bike_proc(void)
{
    if(bike_2AD2_send_flag > 0)
    {
        if(clock_time_exceed(bike_2AD2_send_ticks,800000))
        {
            if(BLE_SUCCESS == bls_att_pushNotifyData(BIKE_2AD2_DP_H,push_buf,bike_2AD2_send_len))
            {
                bls_att_pushNotifyData(BIKE_2AD2_DP_H,push_buf2,bike_2AD2_send_len2);
            }
            bike_2AD2_send_flag--;
            bike_2AD2_send_ticks = clock_time();
        }
    }
}
