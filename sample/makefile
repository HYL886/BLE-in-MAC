# User Parameter
DEVICE_TYPE = BOAT
DEVICE_MODEL = TFB02607061
DEVICE_SW_VERSION = 3.0.0
PRODUCT_MODEL = SRW6000
SYS_BLE_NAME_RANDOM_LEN = 0

SYS_USE_MODU_ONLY = 1


# Debug Parameter
UART_PRINT_DEBUG_ENABLE = 0
BLE_REMOTE_SECURITY_ENABLE = 0

# Default Parameter
BRAND_NAME = ThinkFIT
DEVICE_NAME = TFMODU
DEVICE_HW_VERSION = 1.0
ifeq ($(shell echo "$(DEVICE_SW_VERSION)" | cut -c1),4)
DEVICE_FW_VERSION = 4.0.1.3
SYS_CONFIG_USE_SDK4_EN = 1
else ifeq ($(shell echo "$(DEVICE_SW_VERSION)" | cut -c1),3)
DEVICE_FW_VERSION = 3.4.2.4
SYS_CONFIG_USE_SDK4_EN = 0
else
$(error "Please check if the DEVICE_SW_VERSION is correct!!!")
endif
PRODUCT_CODE = 0x0000

PARAMETER_BRAND_ID = 0x58
ifeq ($(DEVICE_TYPE),TRML)
PARAMETER_TYPE_ID = 0x50
else ifeq ($(DEVICE_TYPE),CRTR)
PARAMETER_TYPE_ID = 0x10
else ifeq ($(DEVICE_TYPE),BIKE)
PARAMETER_TYPE_ID = 0x20
else ifeq ($(DEVICE_TYPE),BOAT)
PARAMETER_TYPE_ID = 0x30
else
$(error "Please check if the DEVICE_TYPE is correct！！！")
endif
PARAMETER_MODE_ID = $(PRODUCT_CODE)

# Select Chip  
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_825x

TEL_CHIP += \
	-DBRAND_NAME=\"$(BRAND_NAME)\" \
	-DDEVICE_TYPE=\"$(DEVICE_TYPE)\" \
	-DDEVICE_NAME=\"$(DEVICE_NAME)\" \
	-DDEVICE_MODEL=\"$(DEVICE_MODEL)\" \
	-DDEVICE_HW_VERSION=\"$(DEVICE_HW_VERSION)\" \
	-DDEVICE_SW_VERSION=\"$(DEVICE_SW_VERSION)\" \
	-DDEVICE_FW_VERSION=\"$(DEVICE_FW_VERSION)\" \
	-DPRODUCT_MODEL=\"$(PRODUCT_MODEL)\" \
	-DPRODUCT_CODE=\"$(PRODUCT_CODE)\" \
	-DSYS_CONFIG_USE_SDK=\"$(SYS_CONFIG_USE_SDK)\" \

TEL_CHIP += \
	-DSYS_BLE_NAME_RANDOM_LEN=$(SYS_BLE_NAME_RANDOM_LEN) \
	-DPARAMETER_BRAND_ID=$(PARAMETER_BRAND_ID) \
	-DPARAMETER_TYPE_ID=$(PARAMETER_TYPE_ID) \
	-DPARAMETER_MODE_ID=$(PARAMETER_MODE_ID) \
	-DUART_PRINT_DEBUG_ENABLE=$(UART_PRINT_DEBUG_ENABLE) \
	-DBLE_REMOTE_SECURITY_ENABLE=$(BLE_REMOTE_SECURITY_ENABLE) \
	-DSYS_USE_MODU_ONLY=$(SYS_USE_MODU_ONLY) \
	-DSYS_CONFIG_USE_SDK4_EN=$(SYS_CONFIG_USE_SDK4_EN) \

# Select link lib
ifeq ($(shell echo "$(DEVICE_SW_VERSION)" | cut -c1),4)
LIBS :=  -llt_8258_m1s1 
TEL_PATH := ../../sdk4
else ifeq ($(shell echo "$(DEVICE_SW_VERSION)" | cut -c1),3)
LIBS :=  -llt_825x -llt_general_stack
TEL_PATH := ../../sdk3
else
$(error "Please check if the LIBS is correct!!!")
endif

COMPILE_TIME = $(shell date +"%Y%m%d%H%M%S")
GIT_REVISION = $(shell git show -s --pretty=format:%h)
# Set Project Name
PROJECT_NAME := $(DEVICE_MODEL)_$(DEVICE_NAME)_$(DEVICE_TYPE)_$(PRODUCT_MODEL)_$(DEVICE_SW_VERSION)_$(COMPILE_TIME)

#########################################################################################
# The following content need not be modified by the user!!!!!							#
#########################################################################################
PROJECT_PATH := .
OUT_PATH :=$(PROJECT_PATH)/out

ifneq ($(TEL_PATH)/make/makefile, $(wildcard $(TEL_PATH)/make/makefile))
$(error "Please check if the TEL_PATH is correct, 请检查 TEL_PATH 是否设置正确！！！")
endif

#include the SDK makefile
-include $(TEL_PATH)/make/makefile
