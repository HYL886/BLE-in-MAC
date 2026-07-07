/********************************************************************************************************
 * @file     app_att.c
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 18, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary property of Telink
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#include "tl_common.h"
#include "ble.h"
#include "ftms.h"
#include "msg.h"
#include "app_att.h"

#include "stack/ble/ble.h"

extern int ble_ota_read(void * p);
extern int ble_ota_write(void * p);

typedef struct
{
  /** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMin;
  /** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMax;
  /** Number of LL latency connection events (0x0000 - 0x03e8) */
  u16 latency;
  /** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
  u16 timeout;
} gap_periConnectParams_t;

static const u16 clientCharacterCfgUUID = GATT_UUID_CLIENT_CHAR_CFG;

static const u16 userdesc_UUID	= GATT_UUID_CHAR_USER_DESC;

static const u16 serviceChangeUUID = GATT_UUID_SERVICE_CHANGE;

static const u16 my_primaryServiceUUID = GATT_UUID_PRIMARY_SERVICE;

static const u16 my_characterUUID = GATT_UUID_CHARACTER;

static const u16 my_devServiceUUID = SERVICE_UUID_DEVICE_INFORMATION;

static const u16 my_2A29UUID = CHARACTERISTIC_UUID_MANU_NAME_STRING;
static const u16 my_2A24UUID = CHARACTERISTIC_UUID_MODEL_NUM_STRING;
static const u16 my_2A25UUID = CHARACTERISTIC_UUID_SERIAL_NUM_STRING;
static const u16 my_2A27UUID = CHARACTERISTIC_UUID_HW_REVISION_STRING;
static const u16 my_2A26UUID = CHARACTERISTIC_UUID_FW_REVISION_STRING;
static const u16 my_2A28UUID = CHARACTERISTIC_UUID_SW_REVISION_STRING;
static const u16 my_2A23UUID = CHARACTERISTIC_UUID_SYSTEM_ID;

static const u16 my_devNameUUID = GATT_UUID_DEVICE_NAME;

static const u16 my_gapServiceUUID = SERVICE_UUID_GENERIC_ACCESS;

static const u16 my_appearanceUIID = GATT_UUID_APPEARANCE;

static const u16 my_periConnParamUUID = GATT_UUID_PERI_CONN_PARAM;

static const u16 my_appearance = GAP_APPEARE_UNKNOWN;

static const u16 my_gattServiceUUID = SERVICE_UUID_GENERIC_ATTRIBUTE;

static const gap_periConnectParams_t my_periConnParameters = {8, 8, 0, 1000};

static u16 serviceChangeVal[2] = {0};

static u8 serviceChangeCCC[2] = {0,0};

extern u8 my_devName[20];

extern u8 my_2A23trs[8];
extern u8 my_2A25trs[13];
extern u8 my_2A29trs[20];
extern u8 my_2A24trs[20];
static const u8 my_2A27trs [] = DEVICE_HW_VERSION;
static const u8 my_2A26trs [] = DEVICE_FW_VERSION;
static const u8 my_2A28trs [] = DEVICE_SW_VERSION;

/******************************************************************/
/*0xFFF0 - 0xFFF2*/
static const u16 mThinkFitComFFF0UUID = 0xFFF0;
static const u16 mThinkFitComFFF1UUID = 0xFFF1;
static const u16 mThinkFitComFFF2UUID = 0xFFF2;

static const u8 mThinkFitComFFF1Val[5] = {
	CHAR_PROP_INDICATE | CHAR_PROP_NOTIFY,
	U16_LO(COMM_FFF1_DP_H), U16_HI(COMM_FFF1_DP_H),
	U16_LO(0xFFF1), U16_HI(0xFFF1)
};
static const u8 mThinkFitComFFF1Data[] = "FFF1";
static const u8 mThinkFitComFFF1Name[] = "Read";
static const u8 mThinkFitComFFF1CCC[2];

static const u8 mThinkFitComFFF2Val[5] = {
	CHAR_PROP_WRITE,
	U16_LO(COMM_FFF2_DP_H), U16_HI(COMM_FFF2_DP_H),
	U16_LO(0xFFF2), U16_HI(0xFFF2)
};
static const u8 mThinkFitComFFF2Data[] = "FFF2";
static const u8 mThinkFitComFFF2Name[] = "Write";
static const u8 mThinkFitComFFF2CCC[2];

// Just for Mearch
/*0x0000 - 0x0001*/
#define MRKS8UUID	0x48,0x43,0x41,0x52,0x45,0x4D,0x88,0x88,0x66,0x66,0x00,0x80,0x55,0x4C,0x55,0x59
#define MRKC0UUID	0x48,0x43,0x41,0x52,0x45,0x4D,0x88,0x88,0x66,0x66,0x00,0x00,0x55,0x4C,0x55,0x59

static const u8 mThinkFitMRKS8UUID[16] = WRAPPING_BRACES(MRKS8UUID);
static const u8 mThinkFitMRKC0UUID[16] = WRAPPING_BRACES(MRKC0UUID);

static const u8 mThinkFitMRKC0Val[19] = {
	CHAR_PROP_INDICATE | CHAR_PROP_WRITE,
	U16_LO(MRK_0000_DP_H), U16_HI(MRK_0000_DP_H),
	MRKC0UUID
};

static const u8 mThinkFitMRKC0Name[] = "MRK-HR";
static const u8 mThinkFitMRKC0Data[] = "MRKC0";
static const u8 mThinkFitMRKC0CCC[2];

/******************************************************************/
static const u16 mThinkFit1826UUID = 0x1826;
static const u16 mThinkFit2ACCUUID = 0x2ACC;
static const u16 mThinkFit2ACDUUID = 0x2ACD;
static const u16 mThinkFit2ACEUUID = 0x2ACE;
static const u16 mThinkFit2AD1UUID = 0x2AD1;
static const u16 mThinkFit2AD2UUID = 0x2AD2;
static const u16 mThinkFit2AD3UUID = 0x2AD3;
static const u16 mThinkFit2AD4UUID = 0x2AD4;
static const u16 mThinkFit2AD5UUID = 0x2AD5;
static const u16 mThinkFit2AD6UUID = 0x2AD6;
static const u16 mThinkFit2AD7UUID = 0x2AD7;
static const u16 mThinkFit2AD8UUID = 0x2AD8;
static const u16 mThinkFit2AD9UUID = 0x2AD9;
static const u16 mThinkFit2ADAUUID = 0x2ADA;

static const u8 mThinkFit2ACCVal[5] = {
	CHAR_PROP_READ,
	U16_LO(FITNESS_2ACC_DP_H), U16_HI(FITNESS_2ACC_DP_H),
	U16_LO(0x2ACC), U16_HI(0x2ACC)
};
static u8 mThinkFit2ACCData[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
void att_set_2ACC_data(u8 *buf, u8 len)
{
    if(len >= 8)
    {
        memcpy(mThinkFit2ACCData,buf,8);
    }
}
static const u8 mThinkFit2ACCName[] = "Fitness Machine Feature";
static const u8 mThinkFit2ACCCCC[2];

static const u8 mThinkFit2ACDVal[5] = {
	CHAR_PROP_NOTIFY,
	U16_LO(TREADMILL_2ACD_DP_H), U16_HI(TREADMILL_2ACD_DP_H),
	U16_LO(0x2ACD), U16_HI(0x2ACD)
};
static const u8 mThinkFit2ACDData[] = "2ACD";
static const u8 mThinkFit2ACDName[] = "Treadmill Data";
static const u8 mThinkFit2ACDCCC[2];

static const u8 mThinkFit2ACEVal[5] = {
	CHAR_PROP_NOTIFY,
	U16_LO(CRTR_2ACE_DP_H), U16_HI(CRTR_2ACE_DP_H),
	U16_LO(0x2ACE), U16_HI(0x2ACE)
};
static const u8 mThinkFit2ACEData[] = "2ACE";
static const u8 mThinkFit2ACEName[] = "Cross Trainer Data";
static const u8 mThinkFit2ACECCC[2];

static const u8 mThinkFit2AD1Val[5] = {
	CHAR_PROP_NOTIFY,
	U16_LO(BOAT_2AD1_DP_H), U16_HI(BOAT_2AD1_DP_H),
	U16_LO(0x2AD1), U16_HI(0x2AD1)
};
static const u8 mThinkFit2AD1Data[] = "2AD1";
static const u8 mThinkFit2AD1Name[] = "Rower Data";
static const u8 mThinkFit2AD1CCC[2];

static const u8 mThinkFit2AD2Val[5] = {
	CHAR_PROP_NOTIFY,
	U16_LO(BIKE_2AD2_DP_H), U16_HI(BIKE_2AD2_DP_H),
	U16_LO(0x2AD2), U16_HI(0x2AD2)
};
static const u8 mThinkFit2AD2Data[] = "2AD2";
static const u8 mThinkFit2AD2Name[] = "Indoor Bike Data";
static const u8 mThinkFit2AD2CCC[2];

static const u8 mThinkFit2AD3Val[5] = {
	CHAR_PROP_READ | CHAR_PROP_NOTIFY,
	U16_LO(FITNESS_2AD3_DP_H), U16_HI(FITNESS_2AD3_DP_H),
	U16_LO(0x2AD3), U16_HI(0x2AD3)
};
static u8 mThinkFit2AD3Data[] = {0x01,0x01};
void att_set_2AD3_data(u8 *buf, u8 len)
{
    if(len >= 2)
    {
        memcpy(mThinkFit2AD3Data,buf,2);
    }
}
static const u8 mThinkFit2AD3Name[] = "Training Status";
static const u8 mThinkFit2AD3CCC[2];

static const u8 mThinkFit2AD4Val[5] = {
	CHAR_PROP_READ,
	U16_LO(FITNESS_2AD4_DP_H), U16_HI(FITNESS_2AD4_DP_H),
	U16_LO(0x2AD4), U16_HI(0x2AD4)
};

static u8 mThinkFit2AD4Data[] = {0x00,0x00,0x00,0x00,0x00,0x00};
void att_set_2AD4_data(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        memcpy(mThinkFit2AD4Data,buf,6);
    }
}
static const u8 mThinkFit2AD4Name[] = "Speed Range";
static const u8 mThinkFit2AD4CCC[2];

static const u8 mThinkFit2AD5Val[5] = {
	CHAR_PROP_READ,
	U16_LO(FITNESS_2AD5_DP_H), U16_HI(FITNESS_2AD5_DP_H),
	U16_LO(0x2AD5), U16_HI(0x2AD5)
};
static u8 mThinkFit2AD5Data[] = {0x00,0x00,0x00,0x00,0x00,0x00};
void att_set_2AD5_data(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        memcpy(mThinkFit2AD5Data,buf,6);
    }
}
static const u8 mThinkFit2AD5Name[] = "Inclination Range";
static const u8 mThinkFit2AD5CCC[2];

static const u8 mThinkFit2AD6Val[5] = {
	CHAR_PROP_READ,
	U16_LO(FITNESS_2AD6_DP_H), U16_HI(FITNESS_2AD6_DP_H),
	U16_LO(0x2AD6), U16_HI(0x2AD6)
};
static u8 mThinkFit2AD6Data[] = {0x00,0x00,0x00,0x00,0x00,0x00};
void att_set_2AD6_data(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        memcpy(mThinkFit2AD6Data,buf,6);
    }
}
static const u8 mThinkFit2AD6Name[] = "Resistance Level Range";
static const u8 mThinkFit2AD6CCC[2];

static const u8 mThinkFit2AD7Val[5] = {
	CHAR_PROP_READ,
	U16_LO(FITNESS_2AD7_DP_H), U16_HI(FITNESS_2AD7_DP_H),
	U16_LO(0x2AD7), U16_HI(0x2AD7)
};
static u8 mThinkFit2AD7Data[] = {0x00,0x00,0xFA,0x00,0x01,0x00};
void att_set_2AD7_data(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        memcpy(mThinkFit2AD7Data,buf,6);
    }
}
static const u8 mThinkFit2AD7Name[] = "Heart Rate Range";
static const u8 mThinkFit2AD7CCC[2];

static const u8 mThinkFit2AD8Val[5] = {
	CHAR_PROP_READ,
	U16_LO(FITNESS_2AD8_DP_H), U16_HI(FITNESS_2AD8_DP_H),
	U16_LO(0x2AD8), U16_HI(0x2AD8)
};
static u8 mThinkFit2AD8Data[] = {0x00,0x00,0xE7,0x03,0x01,0x00};
void att_set_2AD8_data(u8 *buf, u8 len)
{
    if(len >= 6)
    {
        memcpy(mThinkFit2AD8Data,buf,6);
    }
}
static const u8 mThinkFit2AD8Name[] = "Power Range";
static const u8 mThinkFit2AD8CCC[2];

static const u8 mThinkFit2AD9Val[5] = {
	CHAR_PROP_INDICATE | CHAR_PROP_WRITE,
	U16_LO(FITNESS_2AD9_DP_H), U16_HI(FITNESS_2AD9_DP_H),
	U16_LO(0x2AD9), U16_HI(0x2AD9)
};
static const u8 mThinkFit2AD9Data[] = "2AD9";
static const u8 mThinkFit2AD9Name[] = "Fitness Machine Control Point";
static const u8 mThinkFit2AD9CCC[2];

static const u8 mThinkFit2ADAVal[5] = {
	CHAR_PROP_NOTIFY,
	U16_LO(FITNESS_2ADA_DP_H), U16_HI(FITNESS_2ADA_DP_H),
	U16_LO(0x2ADA), U16_HI(0x2ADA)
};
static const u8 mThinkFit2ADAData[] = "2ADA";
static const u8 mThinkFit2ADAName[] = "Fitness Machine Status";
static const u8 mThinkFit2ADACCC[2];

#define HUAWEI_FITNESS_EXT_DATA 	0x59,0x14,0xFB,0x69,0x92,0x52,0x55,0xA3,0xE8,0x11,0x4C,0xC4,0x10,0x2C,0x8D,0xD1
#if(SYS_CONFIG_USE_SDK4_EN == 1)
extern int ftms_HFEDRead(u16 connHandle, rf_packet_att_t *p);
extern int ftms_HFEDWrite(u16 connHandle, rf_packet_att_t *p);
#else
extern int ftms_HFEDRead(void * p);
extern int ftms_HFEDWrite(void * p);
#endif
static const u8 mThinkFitHFEDVal[19] = {
    CHAR_PROP_INDICATE | CHAR_PROP_WRITE,
    U16_LO(FITNESS_HFED_DP_H), U16_HI(FITNESS_HFED_DP_H),
    HUAWEI_FITNESS_EXT_DATA,
};

static const u8 my_HFEDUUID[16] = WRAPPING_BRACES(HUAWEI_FITNESS_EXT_DATA);
static const u8 mThinkFitHFEDData[] = "HFED";
static const u8 mThinkFitHFEDName[] = "Fitness Extension Data";
static const u8 mThinkFitHFEDCCC[2];
static u8 my_HFEDData = 0x00;

/******************************************************************/

//////////////////////// OTA //////////////////////////////////
static const  u8 my_OtaServiceUUID[16]				= WRAPPING_BRACES(TELINK_OTA_UUID_SERVICE);
static const  u8 my_OtaUUID[16]						= WRAPPING_BRACES(TELINK_SPP_DATA_OTA);
_attribute_data_retention_	static 		  u8 my_OtaData 						= 0x00;
_attribute_data_retention_	static 		  u8 otaDataCCC[2] 						= {0,0};
static const  u8 my_OtaName[] 						= {'O', 'T', 'A'};

//// GAP attribute values
static const u8 my_devNameCharVal[5] = {
	CHAR_PROP_READ | CHAR_PROP_NOTIFY,
	U16_LO(GenericAccess_DeviceName_DP_H), U16_HI(GenericAccess_DeviceName_DP_H),
	U16_LO(GATT_UUID_DEVICE_NAME), U16_HI(GATT_UUID_DEVICE_NAME)
};
static const u8 my_appearanceCharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(GenericAccess_Appearance_DP_H), U16_HI(GenericAccess_Appearance_DP_H),
	U16_LO(GATT_UUID_APPEARANCE), U16_HI(GATT_UUID_APPEARANCE)
};
static const u8 my_periConnParamCharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(CONN_PARAM_DP_H), U16_HI(CONN_PARAM_DP_H),
	U16_LO(GATT_UUID_PERI_CONN_PARAM), U16_HI(GATT_UUID_PERI_CONN_PARAM)
};


//// GATT attribute values
static const u8 my_serviceChangeCharVal[5] = {
	CHAR_PROP_INDICATE,
	U16_LO(GenericAttribute_ServiceChanged_DP_H), U16_HI(GenericAttribute_ServiceChanged_DP_H),
	U16_LO(GATT_UUID_SERVICE_CHANGE), U16_HI(GATT_UUID_SERVICE_CHANGE)
};

static const u8 my_F8C1trs[] = BRAND_NAME;
static const u8 my_F8C2trs[] = DEVICE_TYPE;
static const u8 my_F8C3trs[] = DEVICE_NAME;
static const u8 my_F8C4trs[] = DEVICE_MODEL;
static const u8 my_F8C5trs[] = DEVICE_HW_VERSION;
static const u8 my_F8C6trs[] = DEVICE_SW_VERSION;
static const u8 my_F8C7trs[] = PRODUCT_MODEL;
static const u8 my_F8C8trs[] = PRODUCT_CODE;

static const u16 my_F8C0UUID = 0xF8C0;
static const u16 my_F8C1UUID = 0xF8C1;
static const u16 my_F8C2UUID = 0xF8C2;
static const u16 my_F8C3UUID = 0xF8C3;
static const u16 my_F8C4UUID = 0xF8C4;
static const u16 my_F8C5UUID = 0xF8C5;
static const u16 my_F8C6UUID = 0xF8C6;
static const u16 my_F8C7UUID = 0xF8C7;
static const u16 my_F8C8UUID = 0xF8C8;

static const u8 my_F8C1Name[] = "Brand Name";
static const u8 my_F8C2Name[] = "Device Type";
static const u8 my_F8C3Name[] = "Device Name";
static const u8 my_F8C4Name[] = "Device Model";
static const u8 my_F8C5Name[] = "Device HW Version";
static const u8 my_F8C6Name[] = "Device SW Version";
static const u8 my_F8C7Name[] = "Product Model";
static const u8 my_F8C8Name[] = "Product Code";
static const u8 my_F8C1CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C1_DP_H), U16_HI(F8C1_DP_H),
	U16_LO(0xF8C1), U16_HI(0xF8C1)
};

static const u8 my_F8C2CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C2_DP_H), U16_HI(F8C2_DP_H),
	U16_LO(0xF8C2), U16_HI(0xF8C2)
};

static const u8 my_F8C3CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C3_DP_H), U16_HI(F8C3_DP_H),
	U16_LO(0xF8C3), U16_HI(0xF8C3)
};

static const u8 my_F8C4CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C4_DP_H), U16_HI(F8C4_DP_H),
	U16_LO(0xF8C4), U16_HI(0xF8C4)
};

static const u8 my_F8C5CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C5_DP_H), U16_HI(F8C5_DP_H),
	U16_LO(0xF8C5), U16_HI(0xF8C5)
};

static const u8 my_F8C6CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C6_DP_H), U16_HI(F8C6_DP_H),
	U16_LO(0xF8C6), U16_HI(0xF8C6)
};

static const u8 my_F8C7CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C7_DP_H), U16_HI(F8C7_DP_H),
	U16_LO(0xF8C7), U16_HI(0xF8C7)
};

static const u8 my_F8C8CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(F8C8_DP_H), U16_HI(F8C8_DP_H),
	U16_LO(0xF8C8), U16_HI(0xF8C8)
};

//// device Information  attribute values
static const u8 my_2A29CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(DeviceInformation_2A29_DP_H), U16_HI(DeviceInformation_2A29_DP_H),
	U16_LO(CHARACTERISTIC_UUID_MANU_NAME_STRING), U16_HI(CHARACTERISTIC_UUID_MANU_NAME_STRING)
};

static const u8 my_2A24CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(DeviceInformation_2A24_DP_H), U16_HI(DeviceInformation_2A24_DP_H),
	U16_LO(CHARACTERISTIC_UUID_MODEL_NUM_STRING), U16_HI(CHARACTERISTIC_UUID_MODEL_NUM_STRING)
};

static const u8 my_2A25CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(DeviceInformation_2A25_DP_H), U16_HI(DeviceInformation_2A25_DP_H),
	U16_LO(CHARACTERISTIC_UUID_SERIAL_NUM_STRING), U16_HI(CHARACTERISTIC_UUID_SERIAL_NUM_STRING)
};

static const u8 my_2A27CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(DeviceInformation_2A27_DP_H), U16_HI(DeviceInformation_2A27_DP_H),
	U16_LO(CHARACTERISTIC_UUID_HW_REVISION_STRING), U16_HI(CHARACTERISTIC_UUID_HW_REVISION_STRING)
};

static const u8 my_2A26CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(DeviceInformation_2A26_DP_H), U16_HI(DeviceInformation_2A26_DP_H),
	U16_LO(CHARACTERISTIC_UUID_FW_REVISION_STRING), U16_HI(CHARACTERISTIC_UUID_FW_REVISION_STRING)
};

static const u8 my_2A28CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(DeviceInformation_2A28_DP_H), U16_HI(DeviceInformation_2A28_DP_H),
	U16_LO(CHARACTERISTIC_UUID_SW_REVISION_STRING), U16_HI(CHARACTERISTIC_UUID_SW_REVISION_STRING)
};

static const u8 my_2A23CharVal[5] = {
	CHAR_PROP_READ,
	U16_LO(DeviceInformation_2A23_DP_H), U16_HI(DeviceInformation_2A23_DP_H),
	U16_LO(CHARACTERISTIC_UUID_SYSTEM_ID), U16_HI(CHARACTERISTIC_UUID_SYSTEM_ID)
};

//// OTA attribute values
static const u8 my_OtaCharVal[19] = {
	CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY | CHAR_PROP_WRITE,
	U16_LO(OTA_CMD_OUT_DP_H), U16_HI(OTA_CMD_OUT_DP_H),
	TELINK_SPP_DATA_OTA,
};

//HEART RATE
static const u16 mHeartRate180DUUID = 0x180D;
static const u16 mHeartRate2A37UUID = 0x2A37;
static const u16 mHeartRate2A38UUID = 0x2A38;

static const u8 mHeartRate2A37Val[5] = {
	CHAR_PROP_NOTIFY,
	U16_LO(HEARERATE_2A37_DP_H), U16_HI(HEARERATE_2A37_DP_H),
	U16_LO(0x2A37), U16_HI(0x2A37)
};
static const u8 mHeartRate2A37Data[] = "2A37";
static const u8 mHeartRate2A37CCC[2];

static const u8 mHeartRate2A38Val[5] = {
	CHAR_PROP_READ,
	U16_LO(HEARERATE_2A38_DP_H), U16_HI(HEARERATE_2A38_DP_H),
	U16_LO(0x2A38), U16_HI(0x2A38)
};
static const u8 mHeartRate2A38Data[] = {0x04};

// TM : to modify
static const attribute_t default_Attributes[] = {

	{FITNESS_PS_H - 1, 0,0,0,0,0},	// total num of attribute

	// 0001 - 0007  gap
	{7,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gapServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devNameCharVal),(u8*)(&my_characterUUID), (u8*)(my_devNameCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devName), (u8*)(&my_devNameUUID), (u8*)(my_devName), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_appearanceCharVal),(u8*)(&my_characterUUID), (u8*)(my_appearanceCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_appearance), (u8*)(&my_appearanceUIID), 	(u8*)(&my_appearance), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_periConnParamCharVal),(u8*)(&my_characterUUID), (u8*)(my_periConnParamCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_periConnParameters),(u8*)(&my_periConnParamUUID), 	(u8*)(&my_periConnParameters), 0},


	// 0008 - 000b gatt
	{4,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gattServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_serviceChangeCharVal),(u8*)(&my_characterUUID), 		(u8*)(my_serviceChangeCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (serviceChangeVal), (u8*)(&serviceChangeUUID), 	(u8*)(&serviceChangeVal), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof (serviceChangeCCC),(u8*)(&clientCharacterCfgUUID), (u8*)(serviceChangeCCC), 0},


	{25,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&my_F8C0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C1CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1trs),       (u8*)(&my_F8C1UUID),        (u8*)(my_F8C1trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C1Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C2CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2trs),       (u8*)(&my_F8C2UUID),        (u8*)(my_F8C2trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C2Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C3CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3trs),       (u8*)(&my_F8C3UUID),        (u8*)(my_F8C3trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C3Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C4CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4trs),       (u8*)(&my_F8C4UUID),        (u8*)(my_F8C4trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C4Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C5CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5trs),       (u8*)(&my_F8C5UUID),        (u8*)(my_F8C5trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C5Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C6CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6trs),       (u8*)(&my_F8C6UUID),        (u8*)(my_F8C6trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C6Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C7CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7trs),       (u8*)(&my_F8C7UUID),        (u8*)(my_F8C7trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C7Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C8CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8trs),       (u8*)(&my_F8C8UUID),        (u8*)(my_F8C8trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C8Name), 0},


	// 000c - 000e  device Information Service
	{15,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_devServiceUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A29CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A29CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A29trs),(u8*)(&my_2A29UUID), (u8*)(my_2A29trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A24CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A24CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A24trs),(u8*)(&my_2A24UUID), (u8*)(my_2A24trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A25CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A25CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A25trs),(u8*)(&my_2A25UUID), (u8*)(my_2A25trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A27CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A27CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A27trs),(u8*)(&my_2A27UUID), (u8*)(my_2A27trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A26CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A26CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A26trs),(u8*)(&my_2A26UUID), (u8*)(my_2A26trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A28CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A28CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A28trs),(u8*)(&my_2A28UUID), (u8*)(my_2A28trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A23CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A23CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A23trs),(u8*)(&my_2A23UUID), (u8*)(my_2A23trs), 0},

    /*****************************************************************************************************************************/
    /*0xFFF0 - 0xFFF2*/
	{9,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitComFFF0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF1Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Data), (u8*)(&mThinkFitComFFF1UUID),   (u8*)(mThinkFitComFFF1Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF1Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF1CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF1CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF2Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2Data), (u8*)(&mThinkFitComFFF2UUID),   (u8*)(mThinkFitComFFF2Data), (att_readwrite_callback_t)&ble_com_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF2Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF2CCC), 0},

    /*0x8000 0x0000 - 0x0001*/
	{5,ATT_PERMISSIONS_READ,2,16,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitMRKS8UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitMRKC0Val), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(mThinkFitMRKC0Data), (u8*)(&mThinkFitMRKC0UUID),     (u8*)(mThinkFitMRKC0Data), 
        (att_readwrite_callback_t)&MRK_HR_Write},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitMRKC0CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitMRKC0CCC), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitMRKC0Name), 0},

	////////////////////////////////////// OTA /////////////////////////////////////////////////////
	// 001c - 001f
	{5,ATT_PERMISSIONS_READ, 2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_OtaServiceUUID), 0, 0},
	{0,ATT_PERMISSIONS_READ, 2, sizeof(my_OtaCharVal),(u8*)(&my_characterUUID), (u8*)(my_OtaCharVal), 0, 0},				//prop
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_OtaData),(u8*)(&my_OtaUUID),	(&my_OtaData), &otaWrite, NULL},				//value
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(otaDataCCC),(u8*)(&clientCharacterCfgUUID), 	(u8*)(otaDataCCC), 0, 0},				//value
	{0,ATT_PERMISSIONS_READ, 2,sizeof (my_OtaName),(u8*)(&userdesc_UUID), (u8*)(my_OtaName), 0, 0},

    /****************************************HEART RATE**********************************************************************************/
    /*0x180D - 0x2A37,0x2A38*/
	{6,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mHeartRate180DUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A37Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Data), (u8*)(&mHeartRate2A37UUID),   (u8*)(mHeartRate2A37Data), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mHeartRate2A37CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mHeartRate2A37CCC), 0},

  	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A38Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Data), (u8*)(&mHeartRate2A38UUID),   (u8*)(mHeartRate2A38Data), 0},  
};

static const attribute_t treadmill_Attributes[] = {

	{TREADMILL_END_H - 1, 0,0,0,0,0},	// total num of attribute

	// 0001 - 0007  gap
	{7,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gapServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devNameCharVal),(u8*)(&my_characterUUID), (u8*)(my_devNameCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devName), (u8*)(&my_devNameUUID), (u8*)(my_devName), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_appearanceCharVal),(u8*)(&my_characterUUID), (u8*)(my_appearanceCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_appearance), (u8*)(&my_appearanceUIID), 	(u8*)(&my_appearance), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_periConnParamCharVal),(u8*)(&my_characterUUID), (u8*)(my_periConnParamCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_periConnParameters),(u8*)(&my_periConnParamUUID), 	(u8*)(&my_periConnParameters), 0},


	// 0008 - 000b gatt
	{4,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gattServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_serviceChangeCharVal),(u8*)(&my_characterUUID), 		(u8*)(my_serviceChangeCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (serviceChangeVal), (u8*)(&serviceChangeUUID), 	(u8*)(&serviceChangeVal), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof (serviceChangeCCC),(u8*)(&clientCharacterCfgUUID), (u8*)(serviceChangeCCC), 0},

	{25,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&my_F8C0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C1CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1trs),       (u8*)(&my_F8C1UUID),        (u8*)(my_F8C1trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C1Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C2CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2trs),       (u8*)(&my_F8C2UUID),        (u8*)(my_F8C2trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C2Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C3CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3trs),       (u8*)(&my_F8C3UUID),        (u8*)(my_F8C3trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C3Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C4CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4trs),       (u8*)(&my_F8C4UUID),        (u8*)(my_F8C4trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C4Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C5CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5trs),       (u8*)(&my_F8C5UUID),        (u8*)(my_F8C5trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C5Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C6CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6trs),       (u8*)(&my_F8C6UUID),        (u8*)(my_F8C6trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C6Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C7CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7trs),       (u8*)(&my_F8C7UUID),        (u8*)(my_F8C7trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C7Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C8CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8trs),       (u8*)(&my_F8C8UUID),        (u8*)(my_F8C8trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C8Name), 0},

	// 000c - 000e  device Information Service
	{15,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_devServiceUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A29CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A29CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A29trs),(u8*)(&my_2A29UUID), (u8*)(my_2A29trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A24CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A24CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A24trs),(u8*)(&my_2A24UUID), (u8*)(my_2A24trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A25CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A25CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A25trs),(u8*)(&my_2A25UUID), (u8*)(my_2A25trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A27CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A27CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A27trs),(u8*)(&my_2A27UUID), (u8*)(my_2A27trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A26CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A26CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A26trs),(u8*)(&my_2A26UUID), (u8*)(my_2A26trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A28CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A28CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A28trs),(u8*)(&my_2A28UUID), (u8*)(my_2A28trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A23CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A23CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A23trs),(u8*)(&my_2A23UUID), (u8*)(my_2A23trs), 0},

    /*****************************************************************************************************************************/
    /*0xFFF0 - 0xFFF2*/
	{9,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitComFFF0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF1Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Data), (u8*)(&mThinkFitComFFF1UUID),   (u8*)(mThinkFitComFFF1Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF1Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF1CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF1CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF2Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2Data), (u8*)(&mThinkFitComFFF2UUID),   (u8*)(mThinkFitComFFF2Data), (att_readwrite_callback_t)&ble_com_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF2Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF2CCC), 0},

    /*0x8000 0x0000 - 0x0001*/
	{5,ATT_PERMISSIONS_READ,2,16,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitMRKS8UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitMRKC0Val), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(mThinkFitMRKC0Data), (u8*)(&mThinkFitMRKC0UUID),     (u8*)(mThinkFitMRKC0Data), 
        (att_readwrite_callback_t)&MRK_HR_Write},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitMRKC0CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitMRKC0CCC), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitMRKC0Name), 0},

    /*****************************************************************************************************************************/
	////////////////////////////////////// OTA /////////////////////////////////////////////////////
	// 001c - 001f
	{5,ATT_PERMISSIONS_READ, 2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_OtaServiceUUID), 0, 0},
	{0,ATT_PERMISSIONS_READ, 2, sizeof(my_OtaCharVal),(u8*)(&my_characterUUID), (u8*)(my_OtaCharVal), 0, 0},				//prop
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_OtaData),(u8*)(&my_OtaUUID),	(&my_OtaData), &otaWrite, NULL},				//value
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(otaDataCCC),(u8*)(&clientCharacterCfgUUID), 	(u8*)(otaDataCCC), 0, 0},				//value
	{0,ATT_PERMISSIONS_READ, 2,sizeof (my_OtaName),(u8*)(&userdesc_UUID), (u8*)(my_OtaName), 0, 0},

    /****************************************HEART RATE**********************************************************************************/
    /*0x180D - 0x2A37,0x2A38*/
	{6,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mHeartRate180DUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A37Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Data), (u8*)(&mHeartRate2A37UUID),   (u8*)(mHeartRate2A37Data), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mHeartRate2A37CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mHeartRate2A37CCC), 0},

  	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A38Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Data), (u8*)(&mHeartRate2A38UUID),   (u8*)(mHeartRate2A38Data), 0},  

	{45,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFit1826UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ACCVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCData),    (u8*)(&mThinkFit2ACCUUID),      (u8*)(mThinkFit2ACCData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ACCName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ACCCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ACCCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD3Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Data),    (u8*)(&mThinkFit2AD3UUID),      (u8*)(mThinkFit2AD3Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD3Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD3CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD3CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD4Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Data),    (u8*)(&mThinkFit2AD4UUID),      (u8*)(mThinkFit2AD4Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD4Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD4CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD4CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD5Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Data),    (u8*)(&mThinkFit2AD5UUID),      (u8*)(mThinkFit2AD5Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD5Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD5CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD5CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD6Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Data),    (u8*)(&mThinkFit2AD6UUID),      (u8*)(mThinkFit2AD6Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD6Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD6CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD6CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD7Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Data),    (u8*)(&mThinkFit2AD7UUID),      (u8*)(mThinkFit2AD7Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD7Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD7CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD7CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD8Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Data),    (u8*)(&mThinkFit2AD8UUID),      (u8*)(mThinkFit2AD8Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD8Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD8CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD8CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD9Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9Data),    (u8*)(&mThinkFit2AD9UUID),      (u8*)(mThinkFit2AD9Data), (att_readwrite_callback_t)&ftms_2AD9_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD9Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD9CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ADAVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAData),    (u8*)(&mThinkFit2ADAUUID),      (u8*)(mThinkFit2ADAData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ADAName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ADACCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ADACCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFitHFEDVal), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_HFEDData),         (u8*)(&my_HFEDUUID),(&my_HFEDData), &ftms_HFEDWrite, &ftms_HFEDRead},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFitHFEDName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitHFEDCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitHFEDCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACDVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ACDVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACDData),    (u8*)(&mThinkFit2ACDUUID),      (u8*)(mThinkFit2ACDData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACDName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ACDName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ACDCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ACDCCC), 0},
};


static const attribute_t bike_Attributes[] = {

	{BIKE_END_H - 1, 0,0,0,0,0},	// total num of attribute

	// 0001 - 0007  gap
	{7,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gapServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devNameCharVal),(u8*)(&my_characterUUID), (u8*)(my_devNameCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devName), (u8*)(&my_devNameUUID), (u8*)(my_devName), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_appearanceCharVal),(u8*)(&my_characterUUID), (u8*)(my_appearanceCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_appearance), (u8*)(&my_appearanceUIID), 	(u8*)(&my_appearance), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_periConnParamCharVal),(u8*)(&my_characterUUID), (u8*)(my_periConnParamCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_periConnParameters),(u8*)(&my_periConnParamUUID), 	(u8*)(&my_periConnParameters), 0},


	// 0008 - 000b gatt
	{4,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gattServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_serviceChangeCharVal),(u8*)(&my_characterUUID), 		(u8*)(my_serviceChangeCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (serviceChangeVal), (u8*)(&serviceChangeUUID), 	(u8*)(&serviceChangeVal), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof (serviceChangeCCC),(u8*)(&clientCharacterCfgUUID), (u8*)(serviceChangeCCC), 0},

	{25,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&my_F8C0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C1CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1trs),       (u8*)(&my_F8C1UUID),        (u8*)(my_F8C1trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C1Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C2CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2trs),       (u8*)(&my_F8C2UUID),        (u8*)(my_F8C2trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C2Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C3CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3trs),       (u8*)(&my_F8C3UUID),        (u8*)(my_F8C3trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C3Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C4CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4trs),       (u8*)(&my_F8C4UUID),        (u8*)(my_F8C4trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C4Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C5CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5trs),       (u8*)(&my_F8C5UUID),        (u8*)(my_F8C5trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C5Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C6CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6trs),       (u8*)(&my_F8C6UUID),        (u8*)(my_F8C6trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C6Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C7CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7trs),       (u8*)(&my_F8C7UUID),        (u8*)(my_F8C7trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C7Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C8CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8trs),       (u8*)(&my_F8C8UUID),        (u8*)(my_F8C8trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C8Name), 0},

	// 000c - 000e  device Information Service
	{15,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_devServiceUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A29CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A29CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A29trs),(u8*)(&my_2A29UUID), (u8*)(my_2A29trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A24CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A24CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A24trs),(u8*)(&my_2A24UUID), (u8*)(my_2A24trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A25CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A25CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A25trs),(u8*)(&my_2A25UUID), (u8*)(my_2A25trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A27CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A27CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A27trs),(u8*)(&my_2A27UUID), (u8*)(my_2A27trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A26CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A26CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A26trs),(u8*)(&my_2A26UUID), (u8*)(my_2A26trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A28CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A28CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A28trs),(u8*)(&my_2A28UUID), (u8*)(my_2A28trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A23CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A23CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A23trs),(u8*)(&my_2A23UUID), (u8*)(my_2A23trs), 0},

    /*****************************************************************************************************************************/
    /*0xFFF0 - 0xFFF2*/
	{9,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitComFFF0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF1Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Data), (u8*)(&mThinkFitComFFF1UUID),   (u8*)(mThinkFitComFFF1Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF1Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF1CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF1CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF2Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2Data), (u8*)(&mThinkFitComFFF2UUID),   (u8*)(mThinkFitComFFF2Data), (att_readwrite_callback_t)&ble_com_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF2Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF2CCC), 0},

    /*0x8000 0x0000 - 0x0001*/
	{5,ATT_PERMISSIONS_READ,2,16,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitMRKS8UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitMRKC0Val), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(mThinkFitMRKC0Data), (u8*)(&mThinkFitMRKC0UUID),     (u8*)(mThinkFitMRKC0Data), 
        (att_readwrite_callback_t)&MRK_HR_Write},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitMRKC0CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitMRKC0CCC), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitMRKC0Name), 0},

    /*****************************************************************************************************************************/
	////////////////////////////////////// OTA /////////////////////////////////////////////////////
	// 001c - 001f
	{5,ATT_PERMISSIONS_READ, 2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_OtaServiceUUID), 0, 0},
	{0,ATT_PERMISSIONS_READ, 2, sizeof(my_OtaCharVal),(u8*)(&my_characterUUID), (u8*)(my_OtaCharVal), 0, 0},				//prop
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_OtaData),(u8*)(&my_OtaUUID),	(&my_OtaData), &otaWrite, NULL},				//value
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(otaDataCCC),(u8*)(&clientCharacterCfgUUID), 	(u8*)(otaDataCCC), 0, 0},				//value
	{0,ATT_PERMISSIONS_READ, 2,sizeof (my_OtaName),(u8*)(&userdesc_UUID), (u8*)(my_OtaName), 0, 0},

    /****************************************HEART RATE**********************************************************************************/
    /*0x180D - 0x2A37,0x2A38*/
	{6,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mHeartRate180DUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A37Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Data), (u8*)(&mHeartRate2A37UUID),   (u8*)(mHeartRate2A37Data), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mHeartRate2A37CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mHeartRate2A37CCC), 0},

  	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A38Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Data), (u8*)(&mHeartRate2A38UUID),   (u8*)(mHeartRate2A38Data), 0},  

	{45,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFit1826UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ACCVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCData),    (u8*)(&mThinkFit2ACCUUID),      (u8*)(mThinkFit2ACCData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ACCName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ACCCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ACCCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD3Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Data),    (u8*)(&mThinkFit2AD3UUID),      (u8*)(mThinkFit2AD3Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD3Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD3CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD3CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD4Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Data),    (u8*)(&mThinkFit2AD4UUID),      (u8*)(mThinkFit2AD4Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD4Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD4CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD4CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD5Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Data),    (u8*)(&mThinkFit2AD5UUID),      (u8*)(mThinkFit2AD5Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD5Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD5CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD5CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD6Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Data),    (u8*)(&mThinkFit2AD6UUID),      (u8*)(mThinkFit2AD6Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD6Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD6CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD6CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD7Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Data),    (u8*)(&mThinkFit2AD7UUID),      (u8*)(mThinkFit2AD7Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD7Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD7CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD7CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD8Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Data),    (u8*)(&mThinkFit2AD8UUID),      (u8*)(mThinkFit2AD8Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD8Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD8CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD8CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD9Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9Data),    (u8*)(&mThinkFit2AD9UUID),      (u8*)(mThinkFit2AD9Data), (att_readwrite_callback_t)&ftms_2AD9_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD9Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD9CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ADAVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAData),    (u8*)(&mThinkFit2ADAUUID),      (u8*)(mThinkFit2ADAData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ADAName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ADACCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ADACCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFitHFEDVal), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_HFEDData),         (u8*)(&my_HFEDUUID),(&my_HFEDData), &ftms_HFEDWrite, &ftms_HFEDRead},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFitHFEDName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitHFEDCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitHFEDCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD2Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD2Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD2Data),    (u8*)(&mThinkFit2AD2UUID),      (u8*)(mThinkFit2AD2Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD2Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD2Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD2CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD2CCC), 0},
};

static const attribute_t boat_Attributes[] = {

	{BOAT_END_H - 1, 0,0,0,0,0},	// total num of attribute

	// 0001 - 0007  gap
	{7,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gapServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devNameCharVal),(u8*)(&my_characterUUID), (u8*)(my_devNameCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devName), (u8*)(&my_devNameUUID), (u8*)(my_devName), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_appearanceCharVal),(u8*)(&my_characterUUID), (u8*)(my_appearanceCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_appearance), (u8*)(&my_appearanceUIID), 	(u8*)(&my_appearance), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_periConnParamCharVal),(u8*)(&my_characterUUID), (u8*)(my_periConnParamCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_periConnParameters),(u8*)(&my_periConnParamUUID), 	(u8*)(&my_periConnParameters), 0},


	// 0008 - 000b gatt
	{4,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gattServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_serviceChangeCharVal),(u8*)(&my_characterUUID), 		(u8*)(my_serviceChangeCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (serviceChangeVal), (u8*)(&serviceChangeUUID), 	(u8*)(&serviceChangeVal), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof (serviceChangeCCC),(u8*)(&clientCharacterCfgUUID), (u8*)(serviceChangeCCC), 0},

	{25,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&my_F8C0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C1CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1trs),       (u8*)(&my_F8C1UUID),        (u8*)(my_F8C1trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C1Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C2CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2trs),       (u8*)(&my_F8C2UUID),        (u8*)(my_F8C2trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C2Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C3CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3trs),       (u8*)(&my_F8C3UUID),        (u8*)(my_F8C3trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C3Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C4CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4trs),       (u8*)(&my_F8C4UUID),        (u8*)(my_F8C4trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C4Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C5CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5trs),       (u8*)(&my_F8C5UUID),        (u8*)(my_F8C5trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C5Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C6CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6trs),       (u8*)(&my_F8C6UUID),        (u8*)(my_F8C6trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C6Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C7CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7trs),       (u8*)(&my_F8C7UUID),        (u8*)(my_F8C7trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C7Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C8CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8trs),       (u8*)(&my_F8C8UUID),        (u8*)(my_F8C8trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C8Name), 0},

	// 000c - 000e  device Information Service
	{15,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_devServiceUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A29CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A29CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A29trs),(u8*)(&my_2A29UUID), (u8*)(my_2A29trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A24CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A24CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A24trs),(u8*)(&my_2A24UUID), (u8*)(my_2A24trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A25CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A25CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A25trs),(u8*)(&my_2A25UUID), (u8*)(my_2A25trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A27CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A27CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A27trs),(u8*)(&my_2A27UUID), (u8*)(my_2A27trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A26CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A26CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A26trs),(u8*)(&my_2A26UUID), (u8*)(my_2A26trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A28CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A28CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A28trs),(u8*)(&my_2A28UUID), (u8*)(my_2A28trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A23CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A23CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A23trs),(u8*)(&my_2A23UUID), (u8*)(my_2A23trs), 0},

    /*****************************************************************************************************************************/
    /*0xFFF0 - 0xFFF2*/
	{9,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitComFFF0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF1Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Data), (u8*)(&mThinkFitComFFF1UUID),   (u8*)(mThinkFitComFFF1Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF1Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF1CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF1CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF2Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2Data), (u8*)(&mThinkFitComFFF2UUID),   (u8*)(mThinkFitComFFF2Data), (att_readwrite_callback_t)&ble_com_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF2Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF2CCC), 0},

    /*0x8000 0x0000 - 0x0001*/
	{5,ATT_PERMISSIONS_READ,2,16,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitMRKS8UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitMRKC0Val), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(mThinkFitMRKC0Data), (u8*)(&mThinkFitMRKC0UUID),     (u8*)(mThinkFitMRKC0Data), 
        (att_readwrite_callback_t)&MRK_HR_Write},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitMRKC0CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitMRKC0CCC), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitMRKC0Name), 0},

    /*****************************************************************************************************************************/
	////////////////////////////////////// OTA /////////////////////////////////////////////////////
	// 001c - 001f
	{5,ATT_PERMISSIONS_READ, 2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_OtaServiceUUID), 0, 0},
	{0,ATT_PERMISSIONS_READ, 2, sizeof(my_OtaCharVal),(u8*)(&my_characterUUID), (u8*)(my_OtaCharVal), 0, 0},				//prop
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_OtaData),(u8*)(&my_OtaUUID),	(&my_OtaData), &otaWrite, NULL},				//value
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(otaDataCCC),(u8*)(&clientCharacterCfgUUID), 	(u8*)(otaDataCCC), 0, 0},				//value
	{0,ATT_PERMISSIONS_READ, 2,sizeof (my_OtaName),(u8*)(&userdesc_UUID), (u8*)(my_OtaName), 0, 0},

    /****************************************HEART RATE**********************************************************************************/
    /*0x180D - 0x2A37,0x2A38*/
	{6,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mHeartRate180DUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A37Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Data), (u8*)(&mHeartRate2A37UUID),   (u8*)(mHeartRate2A37Data), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mHeartRate2A37CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mHeartRate2A37CCC), 0},

  	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A38Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Data), (u8*)(&mHeartRate2A38UUID),   (u8*)(mHeartRate2A38Data), 0},  

	{45,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFit1826UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ACCVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCData),    (u8*)(&mThinkFit2ACCUUID),      (u8*)(mThinkFit2ACCData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ACCName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ACCCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ACCCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD3Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Data),    (u8*)(&mThinkFit2AD3UUID),      (u8*)(mThinkFit2AD3Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD3Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD3CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD3CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD4Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Data),    (u8*)(&mThinkFit2AD4UUID),      (u8*)(mThinkFit2AD4Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD4Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD4CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD4CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD5Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Data),    (u8*)(&mThinkFit2AD5UUID),      (u8*)(mThinkFit2AD5Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD5Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD5CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD5CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD6Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Data),    (u8*)(&mThinkFit2AD6UUID),      (u8*)(mThinkFit2AD6Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD6Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD6CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD6CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD7Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Data),    (u8*)(&mThinkFit2AD7UUID),      (u8*)(mThinkFit2AD7Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD7Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD7CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD7CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD8Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Data),    (u8*)(&mThinkFit2AD8UUID),      (u8*)(mThinkFit2AD8Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD8Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD8CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD8CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD9Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9Data),    (u8*)(&mThinkFit2AD9UUID),      (u8*)(mThinkFit2AD9Data), (att_readwrite_callback_t)&ftms_2AD9_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD9Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD9CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ADAVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAData),    (u8*)(&mThinkFit2ADAUUID),      (u8*)(mThinkFit2ADAData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ADAName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ADACCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ADACCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFitHFEDVal), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_HFEDData),         (u8*)(&my_HFEDUUID),(&my_HFEDData), &ftms_HFEDWrite, &ftms_HFEDRead},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFitHFEDName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitHFEDCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitHFEDCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD1Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD1Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD1Data),    (u8*)(&mThinkFit2AD1UUID),      (u8*)(mThinkFit2AD1Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD1Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD1Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD1CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD1CCC), 0},
};

static const attribute_t crtr_Attributes[] = {

	{BOAT_END_H - 1, 0,0,0,0,0},	// total num of attribute

	// 0001 - 0007  gap
	{7,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gapServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devNameCharVal),(u8*)(&my_characterUUID), (u8*)(my_devNameCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_devName), (u8*)(&my_devNameUUID), (u8*)(my_devName), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_appearanceCharVal),(u8*)(&my_characterUUID), (u8*)(my_appearanceCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_appearance), (u8*)(&my_appearanceUIID), 	(u8*)(&my_appearance), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_periConnParamCharVal),(u8*)(&my_characterUUID), (u8*)(my_periConnParamCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_periConnParameters),(u8*)(&my_periConnParamUUID), 	(u8*)(&my_periConnParameters), 0},


	// 0008 - 000b gatt
	{4,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gattServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_serviceChangeCharVal),(u8*)(&my_characterUUID), 		(u8*)(my_serviceChangeCharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (serviceChangeVal), (u8*)(&serviceChangeUUID), 	(u8*)(&serviceChangeVal), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof (serviceChangeCCC),(u8*)(&clientCharacterCfgUUID), (u8*)(serviceChangeCCC), 0},

	{25,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&my_F8C0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C1CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1trs),       (u8*)(&my_F8C1UUID),        (u8*)(my_F8C1trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C1Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C1Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C2CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2trs),       (u8*)(&my_F8C2UUID),        (u8*)(my_F8C2trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C2Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C2Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C3CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3trs),       (u8*)(&my_F8C3UUID),        (u8*)(my_F8C3trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C3Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C3Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C4CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4trs),       (u8*)(&my_F8C4UUID),        (u8*)(my_F8C4trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C4Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C4Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C5CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5trs),       (u8*)(&my_F8C5UUID),        (u8*)(my_F8C5trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C5Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C5Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C6CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6trs),       (u8*)(&my_F8C6UUID),        (u8*)(my_F8C6trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C6Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C6Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C7CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7trs),       (u8*)(&my_F8C7UUID),        (u8*)(my_F8C7trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C7Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C7Name), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8CharVal),   (u8*)(&my_characterUUID),   (u8*)(my_F8C8CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8trs),       (u8*)(&my_F8C8UUID),        (u8*)(my_F8C8trs), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(my_F8C8Name),      (u8*)(&userdesc_UUID),      (u8*)(my_F8C8Name), 0},

	// 000c - 000e  device Information Service
	{15,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_devServiceUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A29CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A29CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A29trs),(u8*)(&my_2A29UUID), (u8*)(my_2A29trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A24CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A24CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A24trs),(u8*)(&my_2A24UUID), (u8*)(my_2A24trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A25CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A25CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A25trs),(u8*)(&my_2A25UUID), (u8*)(my_2A25trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A27CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A27CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A27trs),(u8*)(&my_2A27UUID), (u8*)(my_2A27trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A26CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A26CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A26trs),(u8*)(&my_2A26UUID), (u8*)(my_2A26trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A28CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A28CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A28trs),(u8*)(&my_2A28UUID), (u8*)(my_2A28trs), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(my_2A23CharVal),(u8*)(&my_characterUUID), (u8*)(my_2A23CharVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_2A23trs),(u8*)(&my_2A23UUID), (u8*)(my_2A23trs), 0},

    /*****************************************************************************************************************************/
    /*0xFFF0 - 0xFFF2*/
	{9,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitComFFF0UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF1Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Data), (u8*)(&mThinkFitComFFF1UUID),   (u8*)(mThinkFitComFFF1Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF1Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF1Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF1CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF1CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitComFFF2Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2Data), (u8*)(&mThinkFitComFFF2UUID),   (u8*)(mThinkFitComFFF2Data), (att_readwrite_callback_t)&ble_com_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitComFFF2Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitComFFF2Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitComFFF2CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitComFFF2CCC), 0},

    /*0x8000 0x0000 - 0x0001*/
	{5,ATT_PERMISSIONS_READ,2,16,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFitMRKS8UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Val),  (u8*)(&my_characterUUID),       (u8*)(mThinkFitMRKC0Val), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(mThinkFitMRKC0Data), (u8*)(&mThinkFitMRKC0UUID),     (u8*)(mThinkFitMRKC0Data), 
        (att_readwrite_callback_t)&MRK_HR_Write},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitMRKC0CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitMRKC0CCC), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitMRKC0Name), (u8*)(&userdesc_UUID),          (u8*)(mThinkFitMRKC0Name), 0},

    /*****************************************************************************************************************************/
	////////////////////////////////////// OTA /////////////////////////////////////////////////////
	// 001c - 001f
	{5,ATT_PERMISSIONS_READ, 2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_OtaServiceUUID), 0, 0},
	{0,ATT_PERMISSIONS_READ, 2, sizeof(my_OtaCharVal),(u8*)(&my_characterUUID), (u8*)(my_OtaCharVal), 0, 0},				//prop
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_OtaData),(u8*)(&my_OtaUUID),	(&my_OtaData), &otaWrite, NULL},				//value
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(otaDataCCC),(u8*)(&clientCharacterCfgUUID), 	(u8*)(otaDataCCC), 0, 0},				//value
	{0,ATT_PERMISSIONS_READ, 2,sizeof (my_OtaName),(u8*)(&userdesc_UUID), (u8*)(my_OtaName), 0, 0},

    /****************************************HEART RATE**********************************************************************************/
    /*0x180D - 0x2A37,0x2A38*/
	{6,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mHeartRate180DUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A37Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A37Data), (u8*)(&mHeartRate2A37UUID),   (u8*)(mHeartRate2A37Data), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mHeartRate2A37CCC),  (u8*)(&clientCharacterCfgUUID), (u8*)(mHeartRate2A37CCC), 0},

  	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Val),  (u8*)(&my_characterUUID),       (u8*)(mHeartRate2A38Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mHeartRate2A38Data), (u8*)(&mHeartRate2A38UUID),   (u8*)(mHeartRate2A38Data), 0},  
    
	{45,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID),(u8*)(&mThinkFit1826UUID), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ACCVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCData),    (u8*)(&mThinkFit2ACCUUID),      (u8*)(mThinkFit2ACCData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACCName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ACCName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ACCCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ACCCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD3Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Data),    (u8*)(&mThinkFit2AD3UUID),      (u8*)(mThinkFit2AD3Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD3Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD3Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD3CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD3CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD4Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Data),    (u8*)(&mThinkFit2AD4UUID),      (u8*)(mThinkFit2AD4Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD4Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD4Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD4CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD4CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD5Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Data),    (u8*)(&mThinkFit2AD5UUID),      (u8*)(mThinkFit2AD5Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD5Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD5Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD5CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD5CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD6Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Data),    (u8*)(&mThinkFit2AD6UUID),      (u8*)(mThinkFit2AD6Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD6Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD6Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD6CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD6CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD7Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Data),    (u8*)(&mThinkFit2AD7UUID),      (u8*)(mThinkFit2AD7Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD7Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD7Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD7CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD7CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD8Val), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Data),    (u8*)(&mThinkFit2AD8UUID),      (u8*)(mThinkFit2AD8Data), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD8Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD8Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD8CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD8CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Val),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2AD9Val), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9Data),    (u8*)(&mThinkFit2AD9UUID),      (u8*)(mThinkFit2AD9Data), (att_readwrite_callback_t)&ftms_2AD9_receive_callback},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2AD9Name),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2AD9Name), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2AD9CCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2AD9CCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ADAVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAData),    (u8*)(&mThinkFit2ADAUUID),      (u8*)(mThinkFit2ADAData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ADAName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ADAName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ADACCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ADACCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFitHFEDVal), 0},
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_HFEDData),         (u8*)(&my_HFEDUUID),(&my_HFEDData), &ftms_HFEDWrite, &ftms_HFEDRead},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFitHFEDName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFitHFEDName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFitHFEDCCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFitHFEDCCC), 0},

	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACEVal),     (u8*)(&my_characterUUID),       (u8*)(mThinkFit2ACEVal), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACEData),    (u8*)(&mThinkFit2ACEUUID),      (u8*)(mThinkFit2ACEData), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof(mThinkFit2ACEName),    (u8*)(&userdesc_UUID),          (u8*)(mThinkFit2ACEName), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof(mThinkFit2ACECCC),     (u8*)(&clientCharacterCfgUUID), (u8*)(mThinkFit2ACECCC), 0},
};

void my_att_init (void)
{
	bls_att_setAttributeTable ((u8 *)default_Attributes);
}

void treadmill_att_init(void)
{
    bls_att_setAttributeTable ((u8 *)treadmill_Attributes);
}

void bike_att_init(void)
{
    bls_att_setAttributeTable ((u8 *)bike_Attributes);
}

void boat_att_init(void)
{
    bls_att_setAttributeTable ((u8 *)boat_Attributes);
}

void crtr_att_init(void)
{
    bls_att_setAttributeTable ((u8 *)crtr_Attributes);
}
