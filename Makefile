include .color.mk
# Project name
PROJECT = template

# User defined global definitions
DEFS =

# Directory Structure
BINDIR = bin
INCDIR = inc
INCDIR += lib
SRCDIR = src
LIBDIR = lib
OBJDIR = obj

# Uncomment to use the arm-math library (DSP, PID, MATH functions)
# This is pretty space hungy
#USE_ARM_MATH=1

# Startup File
# Choose the correct one from lib/CMSIS/startup
# flash 64KB - 128KB : xb, 256 - 512: xe
# STARTUP = startup_stm32f103xb.s
STARTUP = startup_stm32f103xe.s

# Linker Script, choose one from util/linker or modify one to suit
# The files are fundamentally the same, just the memory mapping differs.
# LDSCRIPT=STM32F103XB_FLASH.ld
LDSCRIPT = STM32F103XE_FLASH.ld

OPENOCD_INTERFACE = stlink
# OPENOCD_INTERFACE = cmsis-dap
OPENOCD_TARGET = stm32f1x
OPENOCD_GDB_PORT = 3333


# Define the processor family
# DEFS += -DSTM32F103xB
DEFS += -DSTM32F103xE

# C compilation flags
CFLAGS = -Wall -Wextra -Os -fno-common -ffunction-sections -fdata-sections -std=c99 -g

# C++ compilation flags
CXXFLAGS = -Wall -Wextra -Os -fno-common -ffunction-sections -fdata-sections -std=c++11 -g

# Linker flags
LDFLAGS = -Wl,--gc-sections --static -Wl,-Map=bin/$(PROJECT).map,--cref

ifdef USE_ARM_MATH
ARM_LIB_DIR = $(LIBDIR)/ARM
ARM_STATIC_LIB = arm_cortexM3l_math
DEFS += -DARM_MATH_CM3
LDFLAGS += --specs=nosys.specs -L$(ARM_LIB_DIR) -l$(ARM_STATIC_LIB)
else
LDFLAGS += --specs=nosys.specs				# This was non.specs, but didn't work for a more complex project 
endif

# MCU FLAGS -> These can be found by sifting through openocd makefiles
# Shouldn't need to be changed over the stm32f1xx family
MCFLAGS = -mcpu=cortex-m3 -mthumb -mlittle-endian -msoft-float -mfix-cortex-m3-ldrd

# GNU ARM Embedded Toolchain
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
LD = arm-none-eabi-ld
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
CP = arm-none-eabi-objcopy
OD = arm-none-eabi-objdump
NM = arm-none-eabi-nm
SIZE = arm-none-eabi-size
A2L = arm-none-eabi-addr2line

# Find source files
ASOURCES = $(LIBDIR)/CMSIS/startup/$(STARTUP)
CSOURCES = $(shell find -L $(SRCDIR) $(LIBDIR) -name '*.c')
CPPSOURCES = $(shell find -L $(SRCDIR) $(LIBDIR) -name '*.cpp')

# Find header directories
INC = $(shell find -L $(INCDIR) -name '*.h' -exec dirname {} \; | uniq)
INC += $(shell find -L $(INCDIR) -name '*.hpp' -exec dirname {} \; | uniq)
INCLUDES = $(INC:%=-I%)

CFLAGS += -c $(MCFLAGS) $(DEFS) $(INCLUDES)
CXXFLAGS += -c $(MCFLAGS) $(DEFS) $(INCLUDES)

AOBJECTS = $(patsubst %,obj/%,$(ASOURCES))
COBJECTS = $(patsubst %,obj/%,$(CSOURCES))
CPPOBJECTS = $(patsubst %,obj/%,$(CPPSOURCES))

OBJECTS = $(AOBJECTS:%.s=%.o) $(COBJECTS:%.c=%.o) $(CPPOBJECTS:%.cpp=%.o)

# Define output files ELF & IHEX
BINELF = $(PROJECT).elf
BINHEX = $(PROJECT).hex

# Additional linker flags
LDFLAGS += -T util/linker/$(LDSCRIPT) $(MCFLAGS) 

# Build Rules
.PHONY: all release debug clean flash erase

all: release

memory: CFLAGS += -g
memory: CXXFLAGS += -g
memory: LDFLAGS += -g -Wl,-Map=$(BINDIR)/$(PROJECT).map
memory:
	@printf "$(GREEN)[Top Memory Use]$(C_NC)\n"
	@$(NM) -A -l -C -td --reverse-sort --size-sort $(BINDIR)/$(BINELF) | head -n10 | cat -n

debug: CFLAGS += -g3
debug: CXXFLAGS += -g3
debug: LDFLAGS += -g3
debug: release

release: $(BINDIR)/$(BINHEX)

$(BINDIR)/$(BINHEX): $(BINDIR)/$(BINELF)
	@$(CP) -O ihex $< $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Converted:$(C_NC)\t%s\n" $<
	@printf "\n$(C_GREEN) [Binary Size]$(C_NC)\n"
	@$(SIZE) $(BINDIR)/$(BINELF)

$(BINDIR)/$(BINELF): $(OBJECTS)
	@mkdir -p $(BINDIR)
	@$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Linked:$(C_NC)\t%s\n" $<

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $< -o $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Compiled:$(C_NC)\t%s\n" $<

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< -o $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Compiled:$(C_NC)\t%s\n" $<

$(OBJDIR)/%.o: %.s
	@printf "$(C_GREEN)[Compiling]$(C_NC)\n"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< -o $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Assembled:$(C_NC)\t%s\n" $<

flash: release
	@printf "\n$(C_GREEN)[Flashing]$(C_NC)"
	@openocd -f interface/$(OPENOCD_INTERFACE).cfg \
		-f target/$(OPENOCD_TARGET).cfg \
        -c "program $(BINDIR)/$(PROJECT).hex verify" \
		-c "reset" \
        -c "exit"
erase:
	@printf "\n$(C_GREEN)[Erasing]$(C_NC)"
	@openocd -f interface/$(OPENOCD_INTERFACE).cfg \
		-f target/$(OPENOCD_TARGET).cfg \
		-c "init" \
		-c "halt" \
		-c "$(OPENOCD_TARGET) mass_erase 0" \
        -c "exit"
clean:
	@rm -rf obj bin

debug: release
	@printf "$(C_GREEN)[Starting OpenOCD...]${C_NC}\n"
	@killall openocd 2>/dev/null || true  					# kill all exist process of OpenOCD
	@openocd -f interface/$(OPENOCD_INTERFACE).cfg \
		-f target/$(OPENOCD_TARGET).cfg \
		-c "gdb_port $(OPENOCD_GDB_PORT)" \
		> /tmp/openocd.log 2>&1 &  							# running in the background
	@printf "$(C_GREEN)[Waiting for OpenOCD to start...]$(C_NC)\n"
	@sleep 2
	@printf "$(C_GREEN)[Starting cgdb...]$(C_NC)\n"
	@cgdb -d arm-none-eabi-gdb $(BINDIR)/$(BINELF) || (killall openocd 2>/dev/null; exit 1)
	@killall openocd 2>/dev/null || true  					# clean after exit

print-%  : ; @echo $* = $($*)

