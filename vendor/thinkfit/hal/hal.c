#include "hal.h"
#include "stack/ble/ble.h"

static const u8 product_code[] = PRODUCT_MODEL;
static const u8 ble_name_random_num_len = SYS_BLE_NAME_RANDOM_LEN;

u8 ble_flags[3] = {0x02,0x01,0x06};
u8 ble_uuids[4] = {0x03,0x03,0x26,0x18};
u8 ble_sdata[7] = {0x06,0x16,0x26,0x18,0x01,0x00,0x00};
u8 ble_lname[20] = {0x00, 0x09};
_attribute_data_retention_ u8 tbl_advData[31];

_attribute_data_retention_ u8 tbl_scanRsp[] = {
#if (APP_YS_ENABLE)
	#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
    0x09, 0xFF, 0x90,0xCB,0x00,0x00,0x00,0x00,0x00,0x00 //TRMI
	#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
	0x09, 0xFF, 0x90,0xCB,0x01,0x00,0x00,0x00,0x00,0x00 //CRTR
	#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
	0x09, 0xFF, 0x90,0xCB,0x02,0x00,0x00,0x00,0x00,0x00 //BIKE
	#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
	0x09, 0xFF, 0x90,0xCB,0x03,0x00,0x00,0x00,0x00,0x00 //BOAT
	#endif
	//0x0D,0xFF,0x78,0x03,0x78,0x13,0xbb,0xc4,0x4E,0x02,0x56,0x65,0x00,0x10
#else
    0x08, 0xFF, 0x7D,0x02,0x01,0x05,0x00,0xFF,0xFF
#endif
};

_attribute_data_retention_ u8 my_devName[21] = {0};

u8 fcs_result(u8 *buf, u8 len)
{
    u8 i = 0,fcs = 0;
    if(len > 0)
    {
        for(i = 0; i < len; i++)
        {
            fcs ^= buf[i];
        }
    }
    return fcs;
}

_attribute_data_retention_ u8 my_2A25trs[13];
_attribute_data_retention_ u8 my_2A23trs[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
_attribute_data_retention_ u8 my_2A29trs[20];  //HW_DEVICE_NAME
_attribute_data_retention_ u8 my_2A24trs[20];  //HW_DEVICE_MODEL
void hal_init_ble_data(void)
{
    /////////////////////// ble_sdata ////////////////////////////////////
	#if (FUNCTION_TYPE == FUNCTION_TYPE_TREADMILL)
		ble_sdata[5] = 0x01;
	#elif (FUNCTION_TYPE == FUNCTION_TYPE_INDOOR_BIKE)
		ble_sdata[5] = 0x20;
	#elif (FUNCTION_TYPE == FUNCTION_TYPE_CROSS_TRAINER)
		ble_sdata[5] = 0x02;
	#elif (FUNCTION_TYPE == FUNCTION_TYPE_ROWER)
		ble_sdata[5] = 0x10;
	#else
		ble_sdata[5] = 0x00;
	#endif

    //////////////////////// BLUE_NAME & ADV_DATA ///////////////////////////////    
    u8 tbl_advData_len = 0;

    memset(tbl_advData,0x00,31);    

    memcpy(tbl_advData + tbl_advData_len,ble_flags,sizeof(ble_flags));
    tbl_advData_len += sizeof(ble_flags);
	memcpy(tbl_advData + tbl_advData_len,ble_sdata,sizeof(ble_sdata));
    tbl_advData_len += sizeof(ble_sdata);
	
	u8 mac_public[6];
	u8 mac_random_static[6];
	
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
	u16 mac_crc16_1 = crc16(&mac_public[0],6);
	u16 mac_crc16_2 = crc16(&mac_public[2],4);
	u16 mac_crc16_3 = crc16(&mac_public[4],2);
	my_2A23trs[0] = mac_crc16_1 & 0xff;
	my_2A23trs[1] = (mac_crc16_1 >> 8) & 0xff;
	my_2A23trs[2] = mac_crc16_2 & 0xff;
	my_2A23trs[3] = (mac_crc16_2 >> 8) & 0xff;
	my_2A23trs[4] = mac_crc16_3 & 0xff;
	my_2A23trs[5] = (mac_crc16_3 >> 8) & 0xff;
	my_2A23trs[6] = 0x00;
	my_2A23trs[7] = 0x00;
	sprintf(my_2A25trs,"%04X%04X%04X",mac_crc16_3,mac_crc16_2,mac_crc16_1);

    if(hal_get_name(ble_lname))
    {
		if(ble_lname[0] < sizeof(ble_lname) - 4)
		{
			memcpy(tbl_advData + tbl_advData_len,ble_uuids,sizeof(ble_uuids));
			tbl_advData_len += sizeof(ble_uuids);
		}
        memcpy(tbl_advData + tbl_advData_len,ble_lname,ble_lname[0]+1);
        memcpy(my_devName,ble_lname+2,ble_lname[0] - 1);
    }else{
		if(sizeof(product_code) - 1 + ble_name_random_num_len < 15)
		{
			memcpy(tbl_advData + tbl_advData_len,ble_uuids,sizeof(ble_uuids));
			tbl_advData_len += sizeof(ble_uuids);
		}
	
        memcpy(my_devName, product_code, sizeof(product_code));
        if(ble_name_random_num_len)
        {
            my_devName[sizeof(product_code) - 1] = '-';
            memcpy(my_devName + sizeof(product_code), &my_2A25trs[12 - ble_name_random_num_len], ble_name_random_num_len);
            ble_lname[0] = 1 + sizeof(product_code) + ble_name_random_num_len;
        }else{
            ble_lname[0] = 1 + sizeof(product_code) - 1 + ble_name_random_num_len;
        }            
        memcpy(ble_lname + 2,my_devName,ble_lname[0] - 1);
        memcpy(tbl_advData + tbl_advData_len,ble_lname,ble_lname[0] + 1);
    } 

    //////////////////////// HW_DEVICE_INFO ///////////////////////////////
    u8 i;
    for(i=0;i<sizeof(product_code);i++)
    {
        if(product_code[i] == '-')
        {
            break;
        }
    }
    if(i != sizeof(product_code))
    {
		if((product_code[0] == 'J')&&(product_code[1] == 'Y')&&(product_code[2] == '-'))
		{
			my_2A29trs[0] = 'X';
			my_2A29trs[1] = 'E';
			my_2A29trs[2] = 'N';
			my_2A29trs[3] = 'J';
			my_2A29trs[4] = 'O';
			my_2A29trs[5] = 'Y';
			memcpy(my_2A24trs,product_code,sizeof(product_code));
		}else{
			memcpy(my_2A29trs,product_code,i);
			if((product_code[0] == 'M')&&(product_code[1] == 'R')&&(product_code[2] == 'K'))
			{
				my_2A24trs[0] = 'M';
				my_2A24trs[1] = 'R';
				my_2A24trs[2] = '-';
				memcpy(my_2A24trs + 3,product_code + i + 1,sizeof(product_code) - i - 2);
			}else{
				memcpy(my_2A24trs,product_code + i + 1,sizeof(product_code) - i - 2);
			}
		}        
    }else{
        memcpy(my_2A29trs,product_code,sizeof(product_code));
        memcpy(my_2A24trs,product_code,sizeof(product_code));
    }
}
void hal_set_ble_data(void)
{
#if (SYS_CONFIG_USE_SDK4_EN == 1)
    blc_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
    blc_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
#else
    bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
    bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
#endif
}

