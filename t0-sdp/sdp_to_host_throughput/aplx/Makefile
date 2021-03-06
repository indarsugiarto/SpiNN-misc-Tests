APP = test_sdp_stream_to_host

# First, define SPINN_DIRS and the correct library
SPINN_DIRS := /opt/spinnaker_tools_134
SPINN_TOOLS := $(SPINN_DIRS)/tools

LIBRARY := $(SPINN_DIRS)/lib/libspin1_api.a
SCRIPT = $(SPINN_DIRS)/lib/sark.lnk

# List of objects making up the application. If there are other files
# in the application, add their object file names to this variable.
OBJECTS = $(APP).o 

ifndef GNUBIN
    GNUBIN := /opt/arm-2013.05/bin
endif

# We mainly use GNU toolchain
GNU := 1

# Prefix for GNU tool binaries
GP  := $(GNUBIN)/arm-none-eabi

INC_DIR := $(SPINN_DIRS)/include
BUILD_DIR = build/
OUTPUT_DIR := ./

#-------------------------------------------------------------------------------

# Set up the various compile/link options for GNU tools

AS := $(GP)-as --defsym GNU=1 -mthumb-interwork -march=armv5te
CC := $(GP)-gcc -c -Os -mthumb-interwork -march=armv5te -std=gnu99 -I $(INC_DIR) -mthumb -DTHUMB
#CC := $(GP)-gcc -c -O3 -mthumb-interwork -march=armv5te -std=gnu99 -I $(INC_DIR) -mthumb -DTHUMB
LD := $(GP)-gcc -T$(SCRIPT) -Wl,-e,cpu_reset -Wl,--gc-sections -Wl,--use-blx
AR := $(GP)-ar -rcs
OC := $(GP)-objcopy
OD := $(GP)-objdump -dxt > $(BUILD_DIR)$(APP).txt

CAT := \cat
RM  := \rm -f
LS  := \ls -l
RM  := \rm -f
MKDIR := \mkdir -p
CP  := \cp -rf

#-------------------------------------------------------------------------------

# Build the application

# Primary target is an APLX file - built from the ELF
$(APP).aplx: prep_dir $(APP).elf
ifeq ($(GNU),1)
	$(OC) -O binary -j RO_DATA -j .ARM.exidx $(BUILD_DIR)$(APP).elf $(BUILD_DIR)RO_DATA.bin
	$(OC) -O binary -j RW_DATA $(BUILD_DIR)$(APP).elf $(BUILD_DIR)RW_DATA.bin
	$(SPINN_TOOLS)/mkbin $(BUILD_DIR)RO_DATA.bin $(BUILD_DIR)RW_DATA.bin > $(BUILD_DIR)$(APP).bin
else
	$(OC) --bin --output $(APP).bin $(APP).elf
endif
	$(SPINN_TOOLS)/mkaplx $(BUILD_DIR)$(APP).elf | $(CAT) - $(BUILD_DIR)$(APP).bin > $(BUILD_DIR)$(APP).aplx
	@$(CP) $(BUILD_DIR)$(APP).aplx $(OUTPUT_DIR)
	@$(LS) $(OUTPUT_DIR)$(APP).aplx

# Build the ELF file
$(APP).elf: $(OBJECTS) $(SCRIPT) $(LIBRARY)
	$(SPINN_TOOLS)/mkbuild $(APP) > $(BUILD_DIR)sark_build.c
	$(CC) $(BUILD_DIR)sark_build.c -o $(BUILD_DIR)sark_build.o
	$(LD) $(LFLAGS) $(addprefix $(BUILD_DIR),$(OBJECTS)) $(BUILD_DIR)sark_build.o $(LIBRARY) -o $(BUILD_DIR)$(APP).elf
	$(OD) $(BUILD_DIR)$(APP).elf


$(APP).o: $(APP).c
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)$@

#-------------------------------------------------------------------------------

prep_dir:
	@$(MKDIR) $(BUILD_DIR)
	@$(MKDIR) $(OUTPUT_DIR)

clean:
	@$(RM) $(BUILD_DIR)*
	@$(RM) $(OUTPUT_DIR)$(APP).aplx

