VENDOR_DIRS := $(shell find ../vendor -maxdepth 4 -type d)
VENDOR_OUTPUT_DIRS = $(patsubst ..%,%, $(VENDOR_DIRS))
OUT_DIR += $(VENDOR_OUTPUT_DIRS)

VENDOR_CFILES := $(foreach dir,$(VENDOR_DIRS),$(wildcard $(dir)/*.c))
VENDOR_INCLUE_PATHS := $(foreach dir,$(VENDOR_DIRS),-I$(dir))
INCLUDE_PATHS += $(VENDOR_INCLUE_PATHS)

VENDOR_OBJS = $(patsubst %.c,%.o, $(VENDOR_CFILES))
VENDOR_OUTPUT_OBJS = $(patsubst ../%.c,$(OUT_PATH)/%.o, $(VENDOR_CFILES))

OBJS += $(VENDOR_OUTPUT_OBJS)

$(OUT_PATH)/vendor/%.o: ../vendor/%.c 
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"

