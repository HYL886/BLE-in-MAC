/********************************************************************************************************
 * @file     app_config.h
 *
 * @brief    for TLSR chips
 *
 * @author   public@telink-semi.com;
 * @date     Sep. 18, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *           The information contained herein is confidential and proprietary property of Telink
 *           Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *           of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *           Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 *           Licensees are granted free, non-transferable use of the information in this
 *           file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#if(SYS_CONFIG_USE_SDK4_EN == 1)
/////////////////// FLASH_LOCK /////////////////////////////////
#define FLASH_LOCK_EN           1

/////////////////// FEATURE SELECT /////////////////////////////////
#define	CONN_MAX_NUM_CONFIG							CONN_MAX_NUM_M1_S1
#define MASTER_MAX_NUM								1
#define SLAVE_MAX_NUM								1

#if(CHIP_TYPE == CHIP_TYPE_825x)
	/* Note: if change to TLSR8258F1K, need pay attention to select the correct flash size */
	#define	FLASH_SIZE_CONFIG		   				FLASH_SIZE_512K  //very important, user need confirm !!!
#elif(CHIP_TYPE == CHIP_TYPE_827x)
	/* Note: if change to TLSR8273, need pay attention to select the correct flash size */
	#define	FLASH_SIZE_CONFIG		   				FLASH_SIZE_1M    //very important, user need confirm !!!
#else
#endif

#define BLE_SLAVE_SMP_ENABLE						0   //1 for smp,  0 no security
#define BLE_MASTER_SMP_ENABLE						0   //1 for smp,  0 no security
#define BLE_MASTER_SIMPLE_SDP_ENABLE				1   //simple service discovery for BLE master
#define BLE_OTA_SERVER_ENABLE						1

#define BLE_APP_PM_ENABLE							0
#define PM_DEEPSLEEP_RETENTION_ENABLE				1

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE							0
#define MASTER_CONNECT_SLAVE_MAC_FILTER_EN			0

/////////////////// Clock  /////////////////////////////////
#define	SYS_CLK_TYPE										SYS_CLK_48M_Crystal

#if(SYS_CLK_TYPE == SYS_CLK_32M_Crystal)
	#define CLOCK_SYS_CLOCK_HZ  							32000000
#elif(SYS_CLK_TYPE == SYS_CLK_48M_Crystal)
	#define CLOCK_SYS_CLOCK_HZ  							48000000
#endif

enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};

/////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
#if (UART_PRINT_DEBUG_ENABLE)
	//the baud rate should not bigger than 1M(system timer clock is constant 16M)
	#define PRINT_BAUD_RATE             		19200//1M baud rate,should Not bigger than 1Mb/s
	#define DEBUG_INFO_TX_PIN           		GPIO_PA0
	#define PULL_WAKEUP_SRC_PA0         		PM_PIN_PULLUP_10K
	#define PA0_OUTPUT_ENABLE         			1
	#define PA0_DATA_OUT                     	1 //must
	#include "application/print/u_printf.h"
#endif

/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE      1
#define WATCHDOG_INIT_TIMEOUT       2000  //ms

#else
/////////////////// FEATURE SELECT /////////////////////////////////
#define BLE_APP_PM_ENABLE                   0
#define PM_DEEPSLEEP_RETENTION_ENABLE       0
#ifndef BLE_APP_SECURITY_ENABLE
#define BLE_APP_SECURITY_ENABLE          	0
#endif
#define BLE_OTA_SERVER_ENABLE				1

#define APP_FLASH_PROTECTION_ENABLE			1
#define APP_BATT_CHECK_ENABLE				0

/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG                   DEEP_ANA_REG0 //u8,can save 8 bit info when deep
#define	LOW_BATT_FLG					    BIT(0) //if 1: low battery
#define CONN_DEEP_FLG                       BIT(1) //if 1: conn deep, 0: adv deep

/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ      48000000

/////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
#ifndef UART_PRINT_DEBUG_ENABLE
#define UART_PRINT_DEBUG_ENABLE 0
#endif
#if (UART_PRINT_DEBUG_ENABLE)
	#define DEBUG_INFO_TX_PIN           	GPIO_PB1
	#define PULL_WAKEUP_SRC_PB1         	PM_PIN_PULLUP_10K
	#define PB1_OUTPUT_ENABLE         		1
	#define PB1_DATA_OUT                    1
#endif

/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE      1
#define WATCHDOG_INIT_TIMEOUT       800  //ms

#endif


/////////////////// function type  /////////////////////////
#define FUNCTION_TYPE_NONE			0
#define FUNCTION_TYPE_TREADMILL		1
#define FUNCTION_TYPE_CROSS_TRAINER	2
#define FUNCTION_TYPE_INDOOR_BIKE	3
#define FUNCTION_TYPE_ROWER			4

#if (PARAMETER_TYPE_ID == 0x50)
#define FUNCTION_TYPE	FUNCTION_TYPE_TREADMILL
#elif (PARAMETER_TYPE_ID == 0x10)
#define FUNCTION_TYPE	FUNCTION_TYPE_CROSS_TRAINER
#elif (PARAMETER_TYPE_ID == 0x20)
#define FUNCTION_TYPE	FUNCTION_TYPE_INDOOR_BIKE
#elif (PARAMETER_TYPE_ID == 0x30)
#define FUNCTION_TYPE	FUNCTION_TYPE_ROWER
#else
#define FUNCTION_TYPE	FUNCTION_TYPE_NONE
#endif
///////////////////////////////////// ATT  HANDLER define ///////////////////////////////////////
typedef enum
{
    ATT_H_START = 0,
    //// Gap ////
    /**********************************************************************************************/
    GenericAccess_PS_H,                     //UUID: 2800,   VALUE: uuid 1800
    GenericAccess_DeviceName_CD_H,          //UUID: 2803,   VALUE:              Prop: Read | Notify
    GenericAccess_DeviceName_DP_H,          //UUID: 2A00,   VALUE: device name
    GenericAccess_Appearance_CD_H,          //UUID: 2803,   VALUE:              Prop: Read
    GenericAccess_Appearance_DP_H,          //UUID: 2A01,   VALUE: appearance
    CONN_PARAM_CD_H,                        //UUID: 2803,   VALUE:              Prop: Read
    CONN_PARAM_DP_H,                        //UUID: 2A04,   VALUE: connParameter

    //// gatt ////
    /**********************************************************************************************/
    GenericAttribute_PS_H,                  //UUID: 2800,   VALUE: uuid 1801
    GenericAttribute_ServiceChanged_CD_H,   //UUID: 2803,   VALUE:              Prop: Indicate
    GenericAttribute_ServiceChanged_DP_H,   //UUID: 2A05,   VALUE: service change
    GenericAttribute_ServiceChanged_CCB_H,  //UUID: 2902,   VALUE: serviceChangeCCC

    F8C0_PS_H,
    F8C1_CD_H, //BRAND NAME
    F8C1_DP_H,
    F8C1_DESC_H,
    F8C2_CD_H, //DEVICE TYPE
    F8C2_DP_H,
    F8C2_DESC_H,
    F8C3_CD_H, //DEVICE NAME
    F8C3_DP_H,
    F8C3_DESC_H,
    F8C4_CD_H, //DEVICE MODEL
    F8C4_DP_H,
    F8C4_DESC_H,
    F8C5_CD_H, //DEVICE HW VERSION
    F8C5_DP_H,
    F8C5_DESC_H,
    F8C6_CD_H, //DEVICE SW VERSION
    F8C6_DP_H,
    F8C6_DESC_H,
    F8C7_CD_H, //PRODUCT MODEL
    F8C7_DP_H,
    F8C7_DESC_H,
    F8C8_CD_H, //PRODUCT CODE
    F8C8_DP_H,
    F8C8_DESC_H,

    //// device information ////
    /**********************************************************************************************/
    DeviceInformation_PS_H,     
    DeviceInformation_2A29_CD_H,
    DeviceInformation_2A29_DP_H,
    DeviceInformation_2A24_CD_H,
    DeviceInformation_2A24_DP_H,
    DeviceInformation_2A25_CD_H,
    DeviceInformation_2A25_DP_H,
    DeviceInformation_2A27_CD_H,
    DeviceInformation_2A27_DP_H,
    DeviceInformation_2A26_CD_H,
    DeviceInformation_2A26_DP_H,
    DeviceInformation_2A28_CD_H,
    DeviceInformation_2A28_DP_H,
    DeviceInformation_2A23_CD_H,
    DeviceInformation_2A23_DP_H,

    // Comm
    COMM_PS_H,
    // Send
    COMM_FFF1_CD_H,
    COMM_FFF1_DP_H,
    COMM_FFF1_DESC_H,
    COMM_FFF1_CCB_H,
    // Receive
    COMM_FFF2_CD_H,
    COMM_FFF2_DP_H,
    COMM_FFF2_DESC_H,
    COMM_FFF2_CCB_H,

    MRK_PS_H,
    MRK_0000_CD_H,
    MRK_0000_DP_H,
    MRK_0000_CCB_H,
    MRK_0000_DESC_H,

    //// Ota ////
	/**********************************************************************************************/
	OTA_PS_H, 								//UUID: 2800, 	VALUE: telink ota service uuid
	OTA_CMD_OUT_CD_H,						//UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp | Notify
	OTA_CMD_OUT_DP_H,						//UUID: telink ota uuid,  VALUE: otaData
	OTA_CMD_INPUT_CCB_H,					//UUID: 2902, 	VALUE: otaDataCCC
	OTA_CMD_OUT_DESC_H,						//UUID: 2901, 	VALUE: otaName

    HEARTRATE_PS_H,              //UUID: 2800, 	VALUE: heart rate service uuid
    HEARTRATE_2A37_CD_H,         //UUID: 2803, 	VALUE:  Prop:  Notify  
    HEARERATE_2A37_DP_H,         //UUID: DATA 	VALUE: heart rate  
    HEARERATE_2A37_CCB_H,        //UUID: 2902, 	VALUE: 2A37CCC
    HEARTRATE_2A38_CD_H,         //UUID: 2803, 	VALUE:  Prop:  read  
    HEARERATE_2A38_DP_H,         //UUID: DATA 	VALUE: heart rate  
    
    FITNESS_PS_H,

    FITNESS_2ACC_CD_H,
    FITNESS_2ACC_DP_H,
    FITNESS_2ACC_DESC_H,
    FITNESS_2ACC_CCB_H,

    FITNESS_2AD3_CD_H,
    FITNESS_2AD3_DP_H,
    FITNESS_2AD3_DESC_H,
    FITNESS_2AD3_CCB_H,

    FITNESS_2AD4_CD_H,
    FITNESS_2AD4_DP_H,
    FITNESS_2AD4_DESC_H,
    FITNESS_2AD4_CCB_H,

    FITNESS_2AD5_CD_H,
    FITNESS_2AD5_DP_H,
    FITNESS_2AD5_DESC_H,
    FITNESS_2AD5_CCB_H,

    FITNESS_2AD6_CD_H,
    FITNESS_2AD6_DP_H,
    FITNESS_2AD6_DESC_H,
    FITNESS_2AD6_CCB_H,

    FITNESS_2AD7_CD_H,
    FITNESS_2AD7_DP_H,
    FITNESS_2AD7_DESC_H,
    FITNESS_2AD7_CCB_H,

    FITNESS_2AD8_CD_H,
    FITNESS_2AD8_DP_H,
    FITNESS_2AD8_DESC_H,
    FITNESS_2AD8_CCB_H,

    FITNESS_2AD9_CD_H,
    FITNESS_2AD9_DP_H,
    FITNESS_2AD9_DESC_H,
    FITNESS_2AD9_CCB_H,

    FITNESS_2ADA_CD_H,
    FITNESS_2ADA_DP_H,
    FITNESS_2ADA_DESC_H,
    FITNESS_2ADA_CCB_H,

    FITNESS_HFED_CD_H,
    FITNESS_HFED_DP_H,
    FITNESS_HFED_DESC_H,
    FITNESS_HFED_CCB_H,

    ATT_END_H,

}ATT_HANDLE;

typedef enum
{
    TREADMILL_2ACD_CD_H = ATT_END_H,
    TREADMILL_2ACD_DP_H,
    TREADMILL_2ACD_DESC_H,
    TREADMILL_2ACD_CCB_H,

    TREADMILL_END_H,
    
}TREADMILL_HANDLE;

typedef enum
{
    BIKE_2AD2_CD_H = ATT_END_H,
    BIKE_2AD2_DP_H,
    BIKE_2AD2_DESC_H,
    BIKE_2AD2_CCB_H,

    BIKE_END_H,
    
}BIKE_HANDLE;

typedef enum
{
    BOAT_2AD1_CD_H = ATT_END_H,
    BOAT_2AD1_DP_H,
    BOAT_2AD1_DESC_H,
    BOAT_2AD1_CCB_H,

    BOAT_END_H,
    
}BOAT_HANDLE;

typedef enum
{
    CRTR_2ACE_CD_H = ATT_END_H,
    CRTR_2ACE_DP_H,
    CRTR_2ACE_DESC_H,
    CRTR_2ACE_CCB_H,

    CRTR_END_H,

}CRTR_HANDLE;

#if(SYS_CONFIG_USE_SDK4_EN == 1)
#include "../../../../sdk4/components/vendor/common/default_config.h"
#else
#include "vendor/common/default_config.h"
#endif

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
