include .color.mk
# Project name
PROJECT = template

# Preprocessor definitions passed to every C compilation.
DEFS =

# Default optimize level
OPTIMIZE = -Os

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

# MCU ?= STM32F103xE
MCU ?= STM32F103xB

ifeq ($(MCU),STM32F103xB) 
# Startup File: Choose the correct one from lib/CMSIS/startup
STARTUP       := startup_stm32f103xb.s
# Linker Script, choose one from util/linker or modify one to suit
LDSCRIPT      := STM32F103XB_FLASH.ld
# Define the processor family
MCU_DEF       := -DSTM32F103xB
FLASH_SIZE_KB := 128
RAM_SIZE_KB   := 20
else ifeq ($(MCU),STM32F103xE)
STARTUP       := startup_stm32f103xe.s
LDSCRIPT      := STM32F103XE_FLASH.ld
MCU_DEF       := -DSTM32F103xE
FLASH_SIZE_KB := 512
RAM_SIZE_KB   := 64
else 
$(error Unsupported MCU "$(MCU)"; supported: STM32F103xB STM32F103xE)
endif

DEFS += $(MCU_DEF)

OPENOCD_INTERFACE = stlink
# OPENOCD_INTERFACE = cmsis-dap
OPENOCD_TARGET = stm32f1x
OPENOCD_GDB_PORT = 3333


# C compilation flags
CFLAGS = -Wall -Wextra $(OPTIMIZE) -fno-common -ffunction-sections -fdata-sections -std=c99

# Generate a .d file for each C object so header changes trigger recompilation.
DEPFLAGS = -MMD -MP -MF $(@:.o=.d) -MT $@

# Linker flags
LDFLAGS = -Wl,--gc-sections --static -Wl,-Map=$(BINDIR)/$(PROJECT).map,--cref -Wl,--no-warn-rwx-segments

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

# Find header directories
INC = $(shell find -L $(INCDIR) -name '*.h' -exec dirname {} \; | uniq)
INCLUDES = $(INC:%=-I%)

CFLAGS += $(MCFLAGS) $(DEFS) $(INCLUDES)

AOBJECTS = $(patsubst %,$(OBJDIR)/%,$(ASOURCES))
COBJECTS = $(patsubst %,$(OBJDIR)/%,$(CSOURCES))
OBJECTS = $(AOBJECTS:%.s=%.o) $(COBJECTS:%.c=%.o)
DEPS := $(COBJECTS:.c=.d)

# Define output files ELF & IHEX
BINELF = $(PROJECT).elf
BINHEX = $(PROJECT).hex

# Additional linker flags
LDFLAGS += -T util/linker/$(LDSCRIPT) $(MCFLAGS) 

# Build Rules
.PHONY: all release debug clean flash erase info compdb
all: release

memory: CFLAGS += -g
memory: LDFLAGS += -g -Wl,-Map=$(BINDIR)/$(PROJECT).map
memory:
	@printf "$(GREEN)[Top Memory Use]$(C_NC)\n"
	@$(NM) -A -l -C -td --reverse-sort --size-sort $(BINDIR)/$(BINELF) | head -n10 | cat -n

release: $(BINDIR)/$(BINHEX)

$(BINDIR)/$(BINHEX): $(BINDIR)/$(BINELF)
	@$(CP) -O ihex $< $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Converted:$(C_NC)\t%s\n" $<
	@printf "\n$(C_GREEN) [Binary Size]$(C_NC)\n"
	@$(SIZE) $(BINDIR)/$(BINELF)

$(BINDIR)/$(BINELF): $(OBJECTS)
	@mkdir -p $(BINDIR)
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Linked:$(C_NC)\t%s\n" $<

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Compiled:$(C_NC)\t%s\n" $<

$(OBJDIR)/%.o: %.s
	@printf "$(C_GREEN)[Compiling]$(C_NC)\n"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
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
	@rm -rf "$(OBJDIR)" "$(BINDIR)"

compdb:
	@tmp="$$(mktemp "$(CURDIR)/.compile_commands.json.XXXXXX")"; \
	trap 'rm -f "$$tmp"' EXIT; \
	compiledb -n -f --full-path -o "$$tmp" make release; \
	grep -q '"file"' "$$tmp" || { \
		printf "error: compiledb did not generate any compilation commands\n" >&2; \
		exit 1; \
	}; \
	mv "$$tmp" "$(CURDIR)/compile_commands.json"
	@printf "$(C_GREEN) [OK] $(C_NC)       $(C_YELLOW) Generated:$(C_NC)\tcompile_commands.json\n"

debug: OPTIMIZE = -O0
debug: CFLAGS += -g3
debug: LDFLAGS += -g3
debug: clean release
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

# Print a Make variable, for example: make print-MCU
print-%:
	@echo $* = $($*)

info:
	@printf "MCU           = %s\n" "$(MCU)"
	@printf "STARTUP       = %s\n" "$(STARTUP)"
	@printf "LDSCRIPT      = %s\n" "$(LDSCRIPT)"
	@printf "MCU_DEF       = %s\n" "$(MCU_DEF)"
	@printf "FLASH_SIZE_KB = %s\n" "$(FLASH_SIZE_KB)"
	@printf "RAM_SIZE_KB   = %s\n" "$(RAM_SIZE_KB)"


# Include compiler-generated header dependencies; ignore them before the first build.
-include $(DEPS)
