##------------------------------------------------------------------------------
##
## Makefile        Makefile for a simple SpiNNaker application
##
## Copyright (C)   The University of Manchester - 2013
##
## Author          Steve Temple, APT Group, School of Computer Science
##
## Email           temples@cs.man.ac.uk
##
##------------------------------------------------------------------------------

# Makefile for a simple SpiNNaker application. This will compile
# a single C source file into an APLX file which can be loaded onto
# SpiNNaker. It will link with either a 'bare' SARK library or a
# combined SARK/API library.

# The options below can be overridden from the command line or via
# environment variables. For example, to compile and link "my_example.c"
# with the ARM tools and generate ARM (as opposed to Thumb) code
#
# make APP=my_example GNU=0 THUMB=0

#---------------------------- Indar -----------------------------
# MATH function:
# Don't forget to include -lm in $(LD) during $(APP).elf building
#----------------------------------------------------------------

# Indar:
# Cannot compile with arm toolchain. It produced:
# Internal fault: [0x44288b:400400] in c_main
# Please contact your supplier.
# or sometimes:
# Internal fault: [0xc31b6d:400400]
# Please contact your supplier.


# Set if we want full optimization (1) or not (0)
OPTIM := 1


SPINN_DIRS := /opt/spinnaker_tools_134
SPINN_TOOLS := /opt/spinnaker_tools_134/tools
GNU_TOOLS := /opt/arm-2013.05/bin/

# Name of app (derived from C source - eg sark.c)

APP := try_dhrystone

# Configuration options

# Set to 1 for GNU tools, 0 for ARM

GNU := 1

# Set to 1 if using SARK/API (0 for SARK)

API := 1

# Set to 1 to make Thumb code (0 for ARM)

THUMB := 0

# Prefix for GNU tool binaries

GP  := $(GNU_TOOLS)arm-none-eabi

# Set to 1 if making a library (advanced!)

LIB := 0

# If SPINN_DIRS is defined, use that to find include and lib directories
# otherwise look two levels up

ifdef SPINN_DIRS
  LIB_DIR := $(SPINN_DIRS)/lib
  INC_DIR := $(SPINN_DIRS)/include
else
  LIB_DIR := ../../lib
  INC_DIR := ../../include
endif

# TAMBAHAN_DIR := /opt/arm-2013.05/lib/gcc/arm-none-eabi/4.7.3/include
TAMBAHAN_DIR := .

#-------------------------------------------------------------------------------

# Set up the various compile/link options for GNU and ARM tools

# GNU tool setup

ifeq ($(GNU),1)
  AS := $(GP)-as --defsym GNU=1 -mthumb-interwork -march=armv5te

  #CA := $(GP)-gcc -c -Os -mthumb-interwork -march=armv5te -std=gnu99 -I $(INC_DIR) -DMSC_CLOCK
ifeq ($(OPTIM),1)
#for optimized:
  CA := $(GP)-gcc -c -O3 -Ofast -mthumb-interwork -march=armv5te -std=gnu99 -I $(INC_DIR)
else
  #for unoptimized:
  #CA := $(GP)-gcc -c -Os -mthumb-interwork -march=armv5te -std=gnu99 -I $(INC_DIR)
  CA := $(GP)-gcc -c -mthumb-interwork -march=armv5te -std=gnu99 -I $(INC_DIR)
endif

  CT := $(CA) -mthumb -DTHUMB

ifeq ($(LIB),1)
  CFLAGS += -fdata-sections -ffunction-sections
endif

ifeq ($(API),1)
#  LIBRARY := -L$(LIB_DIR) -lspin1_api
  LIBRARY := $(LIB_DIR)/libspin1_api.a
else
#  LIBRARY := -L$(LIB_DIR) -lsark
  LIBRARY := $(LIB_DIR)/libsark.a
endif

  SCRIPT := $(LIB_DIR)/sark.lnk

  LD := $(GP)-gcc -T$(SCRIPT) -Wl,-e,cpu_reset -Wl,--gc-sections -Wl,--use-blx 

  AR := $(GP)-ar -rcs
  OC := $(GP)-objcopy
  OD := $(GP)-objdump -dxt > $(APP).txt

# ARM tool setup

else
  AS := armasm --keep --cpu=5te --apcs /interwork

  #CA := armcc -c --c99 --cpu=5te --apcs /interwork --min_array_alignment=4 -I $(INC_DIR) -I $(TAMBAHAN_DIR)
ifeq ($(OPTIM),1)
  #for optimized:
  CA := armcc -c --c99 --cpu=5te --apcs /interwork --min_array_alignment=4 -I $(INC_DIR) -I $(TAMBAHAN_DIR) \
        -O3 -Otime --no_inline --no_multifile
else
  #for unoptimized:
  CA := armcc -c --c99 --cpu=5te --apcs /interwork --min_array_alignment=4 -I $(INC_DIR) -I $(TAMBAHAN_DIR)
endif

  CT := $(CA) --thumb -DTHUMB

ifeq ($(LIB),1)
  CFLAGS += --split_sections
endif

ifeq ($(API),1)
  LIBRARY := $(LIB_DIR)/spin1_api.a
else
  LIBRARY := $(LIB_DIR)/sark.a
endif

  SCRIPT := $(LIB_DIR)/sark.sct

  LD := armlink --scatter=$(SCRIPT) --remove --entry cpu_reset

  AR := armar -rcs
  OC := fromelf
  OD := fromelf -cds --output $(APP).txt

endif

ifeq ($(THUMB),1)
  CC := $(CT)
else
  CC := $(CA)
endif

CAT := \cat
RM  := \rm -f
LS  := \ls -l

#-------------------------------------------------------------------------------

# Build the application

# List of objects making up the application. If there are other files
# in the application, add their object file names to this variable.

OBJECTS := dhry_1.o dhry_2.o


# Primary target is an APLX file - built from the ELF

#  1) Create a binary file which is the concatenation of RO and RW sections
#  2) Make an APLX header from the ELF file with "mkaplx" and concatenate
#     that with the binary to make the APLX file
#  3) Remove temporary files and "ls" the APLX file

$(APP).aplx: $(APP).elf
ifeq ($(GNU),1)
	$(OC) -O binary -j RO_DATA -j .ARM.exidx $(APP).elf RO_DATA.bin
	$(OC) -O binary -j RW_DATA $(APP).elf RW_DATA.bin
	$(SPINN_TOOLS)/mkbin RO_DATA.bin RW_DATA.bin > $(APP).bin
else
	$(OC) --bin --output $(APP).bin $(APP).elf
endif
	$(SPINN_TOOLS)/mkaplx $(APP).elf | $(CAT) - $(APP).bin > $(APP).aplx
	$(RM) $(APP).bin RO_DATA.bin RW_DATA.bin
	$(LS) $(APP).aplx


# Build the ELF file

#  1) Make a "sark_build.c" file containing app. name and build time
#     with "mkbuild" and compile it
#  2) Link application object(s), build file and library to make the ELF
#  3) Tidy up temporaries and create a list file

$(APP).elf: $(OBJECTS) $(SCRIPT) $(LIBRARY) 
	$(SPINN_TOOLS)/mkbuild $(APP) > sark_build.c
	$(CC) sark_build.c
	$(LD) $(LFLAGS) $(OBJECTS) sark_build.o $(LIBRARY) -o $(APP).elf
	$(RM) sark_build.c sark_build.o
	$(OD) $(APP).elf


# Build the main object file. If there are other files in the
# application, place their build dependencies below this one.

dhry_1.o: dhry_1.c dhry.h $(INC_DIR)/spinnaker.h $(INC_DIR)/sark.h $(INC_DIR)/spin1_api.h
	$(CC) $(CFLAGS) dhry_1.c

dhry_2.o: dhry_2.c dhry.h $(INC_DIR)/spinnaker.h $(INC_DIR)/sark.h $(INC_DIR)/spin1_api.h
	$(CC) $(CFLAGS) dhry_2.c

# Tidy and cleaning dependencies

tidy:
	$(RM) $(OBJECTS) $(APP).elf $(APP).txt
clean:
	$(RM) $(OBJECTS) $(APP).elf $(APP).txt $(APP).aplx

#-------------------------------------------------------------------------------
