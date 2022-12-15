# Name of the project, also used for naming the .hex file
M_PROJECT             := teensy3-bcu2
# Used CPU clock
M_CPU_CLOCK           := 72000000
# USB type
M_USB_TYPE            := USB_MXU_SERIAL
# Keyboard layout (what was that for?)
M_LAYOUT              := US_ENGLISH
ifeq ($(shell hostname),rox)
# Arduino version
M_ARDUINO_VERSION     := 10810
# Teensyduino version
M_TEENSYDUINO_VERSION := 148
else
# Arduino version
M_ARDUINO_VERSION     := 20000
# Teensyduino version
M_TEENSYDUINO_VERSION := 157
endif
# Teensy version (3.0, 3.1 or 3.2)
M_TEENSY_VERSION      := 3.2
# Compilation optimization, warning, debug flags
M_OPT_N_WARN          := -Wall -Os
# Uncomment the next line if $(TEENSY3_PATH) contains a full Teensy3 core
# M_REPLACE_CORE := 1
# Comment out the next line to disable auto dependencies generation
M_AUTO_DEPENDENCIES := 1

# Output path for binaries
OUT_PATH          := bin
# Path for sources
SRC_PATH          := src
# Teensy3 override folder
TEENSY3_PATH      := teensy3
# Teensy3 copy folder
TEENSY3_COPY_PATH := teensy3.copy
# Files to remove from base Teensy3
TEENSY3_REMOVED_FILES := usb_serial.c usb_serial2.c usb_serial3.c

################################################################################
#
# Arduino: https://www.arduino.cc/en/Main/Software
# Teensyduino: https://www.pjrc.com/teensy/teensyduino.html
# Source to this Makefile: https://github.com/rjeschke/teensy3-blink
# Reference Makefile: $ARDUINO_HOME/hardware/teensy/avr/cores/teensy3/Makefile
#
################################################################################

# Check if $ARDUINO_HOME is set
ifeq ($(ARDUINO_HOME),)
$(error '$$ARDUINO_HOME is not set')
endif

# Setting up some paths
PWD          := $(realpath $(shell pwd))
LIBRARYPATH  := $(realpath $(ARDUINO_HOME)/libraries)
ifeq ($(M_ARDUINO_VERSION),20000) # FIXME
TOOLSPATH    := $(realpath $(ARDUINO_HOME)/packages/teensy/tools/teensy-tools/1.57.0)
COMPILERPATH := $(realpath $(ARDUINO_HOME)/packages/teensy/tools/teensy-compile/5.4.1/arm/bin)
else
TOOLSPATH    := $(realpath $(ARDUINO_HOME)/hardware/tools)
COMPILERPATH := $(realpath $(ARDUINO_HOME)/hardware/tools/arm/bin)
endif

# Check if tools and compilers exist
ifeq ($(TOOLSPATH),)
$(error "Missing: $(ARDUINO_HOME)/hardware/tools FIXME")
endif
ifeq ($(COMPILERPATH),)
$(error "Missing $(ARDUINO_HOME)/hardware/tools/arm/bin FIXME")
endif

# Setting up linker definitions and other defines
ifeq ($(M_TEENSY_VERSION),3.0)
M_CPU       := __MK20DX128__
M_LINK_DEFS := mk20dx128.ld
M_BOARD     := TEENSY30
else ifeq ($(M_TEENSY_VERSION),3.1)
M_CPU       := __MK20DX256__
M_LINK_DEFS := mk20dx256.ld
M_BOARD     := TEENSY31
else ifeq ($(M_TEENSY_VERSION),3.2)
M_CPU       := __MK20DX256__
M_LINK_DEFS := mk20dx256.ld
M_BOARD     := TEENSY31
else
$(error "Unsupported Teensy version: $(M_TEENSY_VERSION)")
endif

UNIX_TIME = $(shell date '+%s')
CPPFLAGS  = $(M_OPT_N_WARN) -mcpu=cortex-m4 -mthumb -nostdlib
ifneq ($(M_AUTO_DEPENDENCIES),)
CPPFLAGS += -MMD
endif
CPPFLAGS += -DF_CPU=$(M_CPU_CLOCK) -D$(M_CPU) -D$(M_USB_TYPE) -DLAYOUT_$(M_LAYOUT)
CPPFLAGS += -DARDUINO=$(M_ARDUINO_VERSION) -DTEENSYDUINO=$(M_TEENSYDUINO_VERSION)
CPPFLAGS += -DTEENSY_VERSION=$(M_TEENSY_VERSION) -DTEENSY_BOARD=$(M_BOARD)
CXXFLAGS  = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti
ASMFLAGS  = -x assembler-with-cpp
CFLAGS    =
LDFLAGS   = -O -Wl,--gc-sections,--relax,--defsym=__rtc_localtime=$(UNIX_TIME)
LDFLAGS  += -mcpu=cortex-m4 -mthumb --specs=nano.specs
LIBS      = -larm_cortexM4l_math -lm

# Intermediate library for Teensy3 core
LIBTEENSY3          := $(OUT_PATH)/libteensy3.a

# Tools needed
AR                  := $(abspath $(COMPILERPATH))/arm-none-eabi-gcc-ar
CC                  := $(abspath $(COMPILERPATH))/arm-none-eabi-gcc
CXX                 := $(abspath $(COMPILERPATH))/arm-none-eabi-g++
OBJCOPY             := $(abspath $(COMPILERPATH))/arm-none-eabi-objcopy
SIZE                := $(abspath $(COMPILERPATH))/arm-none-eabi-size
TEENSY_POST_COMPILE := $(abspath $(TOOLSPATH))/teensy_post_compile
TEENSY_REBOOT       := $(abspath $(TOOLSPATH))/teensy_reboot

# (Public) target definitions ahead
.PHONY: all clean coreclean depsclean distclean build upload symbols hex eep core

# Same as build
all: src/version.h build

# Compile and link .hex
build: hex

hex: $(OUT_PATH)/$(M_PROJECT).hex

eep: $(OUT_PATH)/$(M_PROJECT).eep

core: $(LIBTEENSY3)

# Clean SOURCE_OBJECTS
clean:
	@rm -rf $(SOURCE_OBJECTS) src/version.h

# Clean LIBTEENSY3 and CORE_LIB_OBJECTS
coreclean:
	@rm -rf $(CORE_LIB_OBJECTS) $(TEENSY3LIB)

# Cleans auto dependencies
depsclean:
	@rm -rf $(OUT_PATH)/*.d

# Clean OUT_PATH and TEENSY3_COPY_PATH
distclean:
	@rm -rf $(OUT_PATH) $(TEENSY3_COPY_PATH)

# Builds and uploads to Teensy3
upload: hex
	@$(TEENSY_POST_COMPILE) -file=$(OUT_PATH)/$(M_PROJECT) -path=$(shell pwd) \
		-tools=$(abspath $(TOOLSPATH)) -board=$(M_BOARD)
	@echo "Rebooting Teensy ..."
	@-$(TEENSY_REBOOT)

# Creates Eclipse CDT symbols
symbols: $(OUT_PATH) $(OUT_PATH)/eclipse_cdt_symbols.xml

# Some functions
relpath2        = $(subst $(1)/,,$(2))
relpath         = $(subst $(PWD)/,,$(1))
list_files      = $(realpath $(shell find $(1) -type f))
filter_sources  = $(filter %.c %.cpp %.S,$(1))
prepare_objects = $(addprefix $(OUT_PATH)/,$(notdir $(addsuffix .o,$(call \
	filter_sources,$(call relpath,$(1))))))

INCLUDE_PATHS  := $(call relpath,$(SRC_PATH))
SOURCE_OBJECTS := $(call prepare_objects,$(call list_files,$(SRC_PATH)))

ifeq ($(M_REPLACE_CORE),)
####
# We're using the vanilla core with optional additions
ifeq ($(M_ARDUINO_VERSION),20000) # FIXME
TEENSY3_CORE_PATH := $(realpath $(ARDUINO_HOME)/packages/teensy/hardware/avr/1.57.0/cores/teensy3)
else
TEENSY3_CORE_PATH := $(realpath $(ARDUINO_HOME)/hardware/teensy/avr/cores/teensy3)
endif
ifeq ($(TEENSY3_CORE_PATH),)
$(error "Missing: $(ARDUINO_HOME)/hardware/teensy/avr/cores/teensy3 FIXME")
endif

TEENSY3_EXCLUDES = Makefile main.cpp

ifeq ($(wildcard $(TEENSY3_PATH)),)
TEENSY3_OVERRIDE_FILES =
else
TEENSY3_OVERRIDE_FILES = $(call relpath2,$(realpath $(TEENSY3_PATH)),$(call \
	list_files,$(TEENSY3_PATH)))
INCLUDE_PATHS += $(TEENSY3_PATH)
TEENSY3_EXCLUDES += $(TEENSY3_OVERRIDE_FILES)
TEENSY3_EXCLUDES += $(TEENSY3_REMOVED_FILES)
endif

TEENSY3_FILES = $(filter-out $(TEENSY3_EXCLUDES),$(call \
	relpath2,$(TEENSY3_CORE_PATH),$(call list_files,$(TEENSY3_CORE_PATH))))

INCLUDE_PATHS += $(TEENSY3_COPY_PATH)

CORE_LIB_OBJECTS = $(call prepare_objects,$(TEENSY3_FILES) \
	$(TEENSY3_OVERRIDE_FILES))

CORE_COPY_DEPENDENCIES = $(addprefix $(TEENSY3_PATH)/,$(TEENSY3_OVERRIDE_FILES)) \
	$(addprefix $(TEENSY3_CORE_PATH)/,$(TEENSY3_FILES))

# (Internal) target definitions ahead
# This one builds TEENSY3_COPY and recursively builds LIBTEENSY3
.ONESHELL:
$(TEENSY3_COPY_PATH): $(CORE_COPY_DEPENDENCIES)
	@for f in $(TEENSY3_FILES);\
		do \
			if [ ! -e $(TEENSY3_COPY_PATH)/$$f ]; \
			then \
				mkdir -p `dirname $(TEENSY3_COPY_PATH)/$$f`; \
				cp -fp $(TEENSY3_CORE_PATH)/$$f $(TEENSY3_COPY_PATH)/$$f; \
			fi \
		done
	@for f in $(TEENSY3_EXCLUDES); \
		do \
			if [ -e $(TEENSY3_COPY_PATH)/$$f ]; then \
				rm $(TEENSY3_COPY_PATH)/$$f; \
			fi \
		done
	@touch $(TEENSY3_COPY_PATH)
	@BUILD_TEENSY3_LIB=1 $(MAKE) $(LIBTEENSY3)

# Only build LIBTEENSY3 if called recursively
ifeq ($(BUILD_TEENSY3_LIB),1)
$(LIBTEENSY3):  $(CORE_LIB_OBJECTS)
else
$(LIBTEENSY3): $(TEENSY3_COPY_PATH) $(CORE_LIB_OBJECTS)
	@echo "Linking $@ ..."
	@$(AR) rcs $@ $(CORE_LIB_OBJECTS)
endif

# Check if user uses linker definitions override
ifeq ($(wildcard $(TEENSY3_PATH)/$(M_LINK_DEFS)),)
LDFLAGS += -T$(TEENSY3_COPY_PATH)/$(M_LINK_DEFS)
else
LDFLAGS += -T$(TEENSY3_PATH)/$(M_LINK_DEFS)
endif

####
else  # ifeq ($(M_REPLACE_CORE),)
####
# We're replacing the whole core

ifeq ($(wildcard $(TEENSY3_PATH)),)
$(error "Missing: $(TEENSY3_PATH)")
endif

CORE_LIB_OBJECTS = $(call prepare_objects,$(call list_files,$(TEENSY3_PATH)))
INCLUDE_PATHS   += $(TEENSY3_PATH)
LDFLAGS         += -T$(TEENSY3_PATH)/$(M_LINK_DEFS)

$(LIBTEENSY3): $(CORE_LIB_OBJECTS)
	@$(AR) rcs $@ $(CORE_LIB_OBJECTS)

####
endif # ifeq ($(M_REPLACE_CORE),)

# Add include folders and set source and object lists
CPPFLAGS    += $(addprefix -I,$(INCLUDE_PATHS))
ALL_OBJECTS  = $(SOURCE_OBJECTS) $(CORE_LIB_OBJECTS)

# Create OUT_PATH
$(OUT_PATH):
	@mkdir -p $(OUT_PATH)

# Creates the .hex from the .elf
$(OUT_PATH)/%.hex: $(OUT_PATH)/%.elf
	@echo "Preparing $@ ..."
	@$(SIZE) $<
	@$(OBJCOPY) -O ihex -R .eeprom $< $@

# Creates the .eep from the .elf
$(OUT_PATH)/%.eep: $(OUT_PATH)/%.elf
	@echo "Preparing $@ ..."
	@$(SIZE) $<
	@$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load \
		--no-change-warnings --change-section-lma .eeprom=0 $< $@

# Make sure OUT_PATH exists
$(CORE_LIB_OBJECTS): | $(OUT_PATH)

# Make sure OUT_PATH exists
$(SOURCE_OBJECTS): | $(OUT_PATH)

# Links the .elf
$(OUT_PATH)/$(M_PROJECT).elf: $(LIBTEENSY3) $(SOURCE_OBJECTS) Makefile
	@echo "Linking "$@" ..."
	@$(CXX) $(LDFLAGS) -o $@ $(SOURCE_OBJECTS) $(LIBTEENSY3) $(LIBS)

# Dedicated rules
define CXX_RULE
$(OUT_PATH)/%.cpp.o: $(1)/%.cpp Makefile
	@echo "Compiling $$< ..."
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $$@ $$<
endef

define CC_RULE
$(OUT_PATH)/%.c.o: $(1)/%.c Makefile
	@echo "Compiling $$< ..."
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $$@ $$<
endef

define ASM_RULE
$(OUT_PATH)/%.S.o: $(1)/%.S Makefile
	@echo "Compiling $$< ..."
	@$(CC) -c $(CPPFLAGS) $(ASM_FLAGS) $(CFLAGS) -o $$@ $$<
endef

# All compile rules
$(foreach tmp,$(INCLUDE_PATHS), \
	$(eval $(call CC_RULE,$(tmp))) \
	$(eval $(call CXX_RULE,$(tmp))) \
	$(eval $(call ASM_RULE,$(tmp))))

# Include auto generated dependencies, don't fail on error
ifneq ($(M_AUTO_DEPENDENCIES),)
-include $(ALL_OBJECTS:.o=.d)
endif

# Eclipse CDT symbol creation
write_symbols = @echo "<macro><name>F_CPU</name><value>$(M_CPU_CLOCK)</value></macro>" \
	"<macro><name>$(M_USB_TYPE)</name><value/></macro>" \
	"<macro><name>$(M_CPU)</name><value/></macro>" \
	"<macro><name>LAYOUT_$(M_LAYOUT)</name><value/></macro>" \
	"<macro><name>ARDUINO</name><value>$(M_ARDUINO_VERSION)</value></macro>" \
	"<macro><name>TEENSYDUINO</name><value>$(M_TEENSYDUINO_VERSION)</value></macro>" \
	"<macro><name>TEENSY_VERSION</name><value>$(M_TEENSY_VERSION)</value></macro>" \
	"<macro><name>TEENSY_BOARD</name><value>$(M_BOARD)</value></macro>" \
	"<macro><name>NULL</name><value>0</value></macro>" \
	>> $(1)

.ONESHELL:
$(OUT_PATH)/eclipse_cdt_symbols.xml: Makefile $(OUT_PATH)
	@echo '<?xml version="1.0" encoding="UTF-8"?>' > $@
	@echo '<cdtprojectproperties>' >> $@
	@echo '<section name="org.eclipse.cdt.internal.ui.wizards.settingswizards.Macros">' >> $@
	@echo '<language name="C++ Source File">' >> $@
	$(call write_symbols,$@)
	@echo '</language>' >> $@
	@echo '<language name="C Source File">' >> $@
	$(call write_symbols,$@)
	@echo '</language>' >> $@
	@echo '</section>' >> $@
	@echo '</cdtprojectproperties>' >> $@

src/version.h:	.git/HEAD .git/index
	@echo "const char *gitversion = \"$(shell scripts/gitversion)\";" > $@
