# Name of the project, also used for naming the .hex file
M_PROJECT             := teensy3-blinky
# Used CPU clock
M_CPU_CLOCK           := 72000000
# USB type
M_USB_TYPE            := USB_SERIAL
# Keyboard layout (what was that for?=
M_LAYOUT              := US_ENGLISH
# Arduino version
M_ARDUINO_VERSION     := 10605
# Teensyduino version
M_TEENSYDUINO_VERSION := 125
# Teensy version (3.0, 3.1 or 3.2)
M_TEENSY_VERSION      := 3.2
# Compilation optimization flags
M_OPTIMIZATIONS       := -O3

# Output path for binaries
OUT_PATH              := bin
# Path for sources
SRC_PATH              := src
# Teensy3 override folder
TEENSY3_PATH          := teensy3
# Teensy3 copy folder
TEENSY3_COPY          := teensy3.copy

################################################################################
#
# Arduino: https://www.arduino.cc/en/Main/Software
# Teensyduino: https://www.pjrc.com/teensy/teensyduino.html
# Original Makefile: https://github.com/rjeschke/teensy3-blink
#
################################################################################

# Setting up some paths
TOOLSPATH             := $(realpath $(ARDUINO_HOME)/hardware/tools)
LIBRARYPATH           := $(realpath $(ARDUINO_HOME)/libraries)
COMPILERPATH          := $(realpath $(ARDUINO_HOME)/hardware/tools/arm/bin)
TEENSY3_CORE_PATH     := $(realpath $(ARDUINO_HOME)/hardware/teensy/avr/cores/teensy3)
TEENSY3_OVERRIDE_PATH := $(realpath ./$(TEENSY3_PATH))

# Setting up linker definitions and M_CPU define
ifeq ($(M_TEENSY_VERSION),3.0)
	M_CPU       := __MK20DX128__
	M_LINK_DEFS := mk20dx128.ld
else ifeq ($(M_TEENSY_VERSION),3.1)
	M_CPU       := __MK20DX256__
	M_LINK_DEFS := mk20dx256.ld
else ifeq ($(M_TEENSY_VERSION),3.2)
	M_CPU       := __MK20DX256__
	M_LINK_DEFS := mk20dx256.ld
else
	$(error "Unsupported Teensy version: $(M_TEENSY)")
endif

# Putting the flags together
CPPFLAGS  = -Wall $(M_OPTIMIZATIONS) -mcpu=cortex-m4 -mthumb -nostdlib -MMD
CPPFLAGS += -DF_CPU=$(M_CPU_CLOCK) -D$(M_USB_TYPE) -DLAYOUT_$(M_LAYOUT)
CPPFLAGS += -DUSING_MAKEFILE -D$(M_CPU) -DARDUINO=$(M_ARDUINO_VERSION)
CPPFLAGS += -DTEENSYDUINO=$(M_TEENSYDUINO_VERSION)
CXXFLAGS  = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti
CFLAGS    =
LDFLAGS   = -Os -Wl,--gc-sections,--defsym=__rtc_localtime=0 --specs=nano.specs
LDFLAGS  += -mcpu=cortex-m4 -mthumb
LIBS      = -lm

# Simple `find` based directory lister
listfiles = $(foreach tmp,$(shell find $(1) -type "f"),$(subst $(1),,$(tmp)))

# Check if SRC_PATH and TEENSY3_CORE_PATH exist
ifeq ($(wildcard $(SRC_PATH)),)
	$(error "Missing SRC_PATH")
endif
ifeq ($(wildcard $(TEENSY3_CORE_PATH)),)
	$(error "Missing Teensy3 core files")
endif

# Check if user uses linker definitions override
ifeq ($(wildcard $(TEENSY3_OVERRIDE_PATH)/$(M_LINK_DEFS)),)
	LDFLAGS  += -T$(TEENSY3_COPY)/$(M_LINK_DEFS)
else
	LDFLAGS  += -T$(TEENSY3_OVERRIDE_PATH)/$(M_LINK_DEFS)
endif

# Files and folders
INCLUDE_FOLDERS   := $(SRC_PATH) $(TEENSY3_COPY)
CORE_FILES        := $(call listfiles,$(TEENSY3_CORE_PATH)/)
CORE_EXCLUDES     := main.cpp Makefile
ALL_TEENSY3_FILES := $(addprefix $(TEENSY3_CORE_PATH)/,$(CORE_FILES))

# Check if Teensy3 override is in place
ifneq ($(wildcard $(TEENSY3_OVERRIDE_PATH)),)
	CORE_OVERRIDES = $(call listfiles,$(TEENSY3_OVERRIDE_PATH)/)
	CORE_EXCLUDES += $(CORE_OVERRIDES)
	ALL_TEENSY3_FILES += $(addprefix $(TEENSY3_OVERRIDE_PATH)/,$(CORE_OVERRIDES))
	INCLUDE_FOLDERS += $(TEENSY3_PATH)
else
	CORE_OVERRIDES =
endif

# Add include folders and set source and object lists
CPPFLAGS        += $(addprefix -I,$(INCLUDE_FOLDERS))
TEENSY_SOURCES  := $(filter %.c %cpp,$(notdir $(filter-out \
  $(CORE_EXCLUDES),$(CORE_FILES)) $(CORE_OVERRIDES)))
TEENSY_OBJECTS  := $(addprefix $(OUT_PATH)/,$(patsubst %.cpp,%.o,$(patsubst \
  %.c,%.o,$(TEENSY_SOURCES))))
SOURCES         := $(filter %.c %cpp,$(notdir $(call listfiles,$(SRC_PATH))))
SOURCE_OBJECTS  := $(addprefix $(OUT_PATH)/,$(patsubst %.cpp,%.o,$(patsubst \
  %.c,%.o,$(SOURCES))))

# Intermediate library for Teensy3 core
LIBTEENSY3          := $(OUT_PATH)/libteensy3.a

# Tools needed
AR                  := $(abspath $(COMPILERPATH))/arm-none-eabi-ar
CC                  := $(abspath $(COMPILERPATH))/arm-none-eabi-gcc
CXX                 := $(abspath $(COMPILERPATH))/arm-none-eabi-g++
OBJCOPY             := $(abspath $(COMPILERPATH))/arm-none-eabi-objcopy
SIZE                := $(abspath $(COMPILERPATH))/arm-none-eabi-size
TEENSY_POST_COMPILE := $(abspath $(TOOLSPATH))/teensy_post_compile
TEENSY_REBOOT       := $(abspath $(TOOLSPATH))/teensy_reboot

# (Public) target definitions ahead
.PHONY: all clean clean-all build upload symbols

# Same as build
all: $(OUT_PATH)/$(M_PROJECT).hex

# Compile and link .hex
build: $(OUT_PATH)/$(M_PROJECT).hex

# Clean OUT_PATH
clean:
	@rm -rf $(OUT_PATH)

# Clean OUT_PATH and TEENSY3_COPY
distclean: clean
	@rm -rf $(TEENSY3_COPY)

# Builds and uploads to Teensy3
upload: $(OUT_PATH)/$(M_PROJECT).hex
	@$(TEENSY_POST_COMPILE) -file=$(OUT_PATH)/$(M_PROJECT) -path=$(shell pwd) \
	  -tools=$(abspath $(TOOLSPATH))
	@echo "Rebooting Teensy ..."
	@-$(TEENSY_REBOOT)

# Creates Eclipse CDT symbols
symbols: $(OUT_PATH) $(OUT_PATH)/eclipse_cdt_symbols.xml

# (Internal) target definitions ahead
# This one builds TEENSY3_COPY and recursively builds LIBTEENSY3
$(TEENSY3_COPY): $(ALL_TEENSY3_FILES)
	@for f in $(filter-out $(CORE_EXCLUDES),$(CORE_FILES));\
    do \
      if [ ! -e $(TEENSY3_COPY)/$$f ]; \
      then \
        mkdir -p `dirname $(TEENSY3_COPY)/$$f`; \
        cp -fp $(TEENSY3_CORE_PATH)/$$f $(TEENSY3_COPY)/$$f; \
      fi \
    done
	@for f in $(CORE_EXCLUDES); \
    do \
      if [ -e $(TEENSY3_COPY)/$$f ]; then \
        rm $(TEENSY3_COPY)/$$f; \
      fi \
    done
	@touch $(TEENSY3_COPY)
	@BUILD_TEENSY3_LIB=1 $(MAKE) $(LIBTEENSY3)

# Create OUT_PATH
$(OUT_PATH):
	@mkdir -p $(OUT_PATH)

# Creates the .hex from the .elf
$(OUT_PATH)/%.hex: $(OUT_PATH)/%.elf
	@echo "Preparing "$(notdir $<)" ..."
	@$(SIZE) $<
	@$(OBJCOPY) -O ihex -R .eeprom $< $@

# Make sure OUT_PATH exists
$(TEENSY_OBJECTS): | $(OUT_PATH)

# Make sure OUT_PATH exists
$(SOURCE_OBJECTS): | $(OUT_PATH)

# Only build LIBTEENSY3 if called recursively
ifeq ($(BUILD_TEENSY3_LIB),1)
$(LIBTEENSY3): $(TEENSY_OBJECTS)
	@$(AR) rcs $@ $(TEENSY_OBJECTS)
else
$(LIBTEENSY3): $(TEENSY3_COPY)
endif

# Links the .elf
$(OUT_PATH)/$(M_PROJECT).elf: $(LIBTEENSY3) $(SOURCE_OBJECTS)
	@echo "Linking "$@" ..."
	@$(CXX) $(LDFLAGS) -o $@ $(SOURCE_OBJECTS) $(LIBS) $(LIBTEENSY3)

# Specialized .c/.cpp rules for all source folders
ifneq ($(wildcard $(TEENSY3_OVERRIDE_PATH)),)
$(OUT_PATH)/%.o: $(TEENSY3_OVERRIDE_PATH)/%.c
	@echo "Compiling "$(notdir $<)" ..."
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(OUT_PATH)/%.o: $(TEENSY3_OVERRIDE_PATH)/%.cpp
	@echo "Compiling "$(notdir $<)" ..."
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<
endif

$(OUT_PATH)/%.o: $(SRC_PATH)/%.c
	@echo "Compiling "$(notdir $<)" ..."
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(OUT_PATH)/%.o: $(SRC_PATH)/%.cpp
	@echo "Compiling "$(notdir $<)" ..."
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

$(OUT_PATH)/%.o: $(TEENSY3_COPY)/%.c
	@echo "Compiling "$(notdir $<)" ..."
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(OUT_PATH)/%.o: $(TEENSY3_COPY)/%.cpp
	@echo "Compiling "$(notdir $<)" ..."
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

# Include auto generated dependencies, don't fail on error
-include $(ALL_OBJECTS:.o=.d)

# Eclipse CDT symbol creator
$(OUT_PATH)/eclipse_cdt_symbols.xml: Makefile $(OUT_PATH)
	@echo '<?xml version="1.0" encoding="UTF-8"?>' > $@
	@echo '<cdtprojectproperties>' >> $@
	@echo '<section name="org.eclipse.cdt.internal.ui.wizards.settingswizards.Macros">' >> $@
	@echo '<language name="C++ Source File">' >> $@
	@echo "<macro><name>F_CPU</name><value>$(M_CPU_CLOCK)</value></macro>" >> $@
	@echo "<macro><name>$(M_USB_TYPE)</name><value/></macro>" >> $@
	@echo "<macro><name>$(M_CPU)</name><value/></macro>" >> $@
	@echo "<macro><name>LAYOUT_$(M_LAYOUT)</name><value/></macro>" >> $@
	@echo "<macro><name>USING_MAKEFILE</name><value/></macro>" >> $@
	@echo "<macro><name>ARDUINO</name><value>$(M_ARDUINO_VERSION)</value></macro>" >> $@
	@echo "<macro><name>TEENSYDUINO</name><value>$(M_TEENSYDUINO_VERSION)</value></macro>" >> $@
	@echo '</language>' >> $@
	@echo '<language name="C Source File">' >> $@
	@echo "<macro><name>F_CPU</name><value>$(M_CPU_CLOCK)</value></macro>" >> $@
	@echo "<macro><name>$(M_USB_TYPE)</name><value/></macro>" >> $@
	@echo "<macro><name>$(M_CPU)</name><value/></macro>" >> $@
	@echo "<macro><name>LAYOUT_$(M_LAYOUT)</name><value/></macro>" >> $@
	@echo "<macro><name>USING_MAKEFILE</name><value/></macro>" >> $@
	@echo "<macro><name>ARDUINO</name><value>$(M_ARDUINO_VERSION)</value></macro>" >> $@
	@echo "<macro><name>TEENSYDUINO</name><value>$(M_TEENSYDUINO_VERSION)</value></macro>" >> $@
	@echo '</language>' >> $@
	@echo '</section>' >> $@
	@echo '</cdtprojectproperties>' >> $@

