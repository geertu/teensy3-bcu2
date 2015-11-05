M_PROJECT             := teensy3-blinky
M_CPU_CLOCK           := 72000000
M_USB_TYPE            := USB_SERIAL
M_LAYOUT              := US_ENGLISH
M_ARDUINO_VERSION     := 10605
M_TEENSYDUINO_VERSION := 125
M_CPU                 := __MK20DX256__
M_OPTIMIZATIONS       := -g -Os

OUT_PATH     := bin
SRC_PATH     := src
TEENSY3_PATH := teensy3
TEENSY3_COPY := teensy3.copy

TOOLSPATH    := $(ARDUINO_HOME)/hardware/tools
LIBRARYPATH  := $(ARDUINO_HOME)/libraries
COMPILERPATH := $(ARDUINO_HOME)/hardware/tools/arm/bin

TEENSY3_CORE_PATH     := $(realpath $(ARDUINO_HOME)/hardware/teensy/avr/cores/teensy3)
TEENSY3_OVERRIDE_PATH := $(realpath ./$(TEENSY3_PATH))

OPTIONS   = -DF_CPU=$(M_CPU_CLOCK) -D$(M_USB_TYPE) -DLAYOUT_$(M_LAYOUT)
OPTIONS  += -DUSING_MAKEFILE -D$(M_CPU) -DARDUINO=$(M_ARDUINO_VERSION)
OPTIONS  += -DTEENSYDUINO=$(M_TEENSYDUINO_VERSION)
CPPFLAGS  = -Wall $(M_OPTIMIZATIONS) -mcpu=cortex-m4 -mthumb -nostdlib
CPPFLAGS += -MMD $(OPTIONS) $(addprefix -I,src $(TEENSY3_COPY))
CXXFLAGS  = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti
CFLAGS    =
LDFLAGS   = -Os -Wl,--gc-sections,--defsym=__rtc_localtime=0 --specs=nano.specs
LDFLAGS  += -mcpu=cortex-m4 -mthumb -T$(TEENSY3_COPY)/mk20dx256.ld
LIBS      = -lm

listfiles = $(foreach tmp,$(shell find $(1) -type "f"),$(subst $(1),,$(tmp)))

EXCLUDED_TEENSY3_CORE_FILES := main.cpp
ifeq ($(wildcard $(TEENSY3_OVERRIDE_PATH)),)
	TEENSY3_OVERRIDE_FILES =
else
	TEENSY3_OVERRIDE_FILES = $(call listfiles,$(TEENSY3_OVERRIDE_PATH)/)
endif
TEENSY3_CORE_FILES := $(filter-out $(EXCLUDED_TEENSY3_CORE_FILES) \
  $(TEENSY3_OVERRIDE_FILES),$(call listfiles,$(TEENSY3_CORE_PATH)/))
ALL_TEENSY3_FILES  := $(addprefix $(TEENSY3_CORE_PATH)/,$(TEENSY3_CORE_FILES)) \
  $(addprefix $(TEENSY3_OVERRIDE_PATH)/,$(TEENSY3_OVERRIDE_FILES))
TEENSY3_OBJECTS    := $(addprefix $(OUT_PATH)/,$(patsubst %.cpp,%.o,$(patsubst \
  %.c,%.o,$(filter %.c %.cpp,$(TEENSY3_CORE_FILES)) \
  $(filter %.c %.cpp,$(TEENSY3_OVERRIDE_FILES)))))

SRC_FILES    := $(call listfiles,$(realpath $(SRC_PATH))/)
SRC_OBJECTS  := $(addprefix $(OUT_PATH)/,$(patsubst %.cpp,%.o,$(patsubst \
  %.c,%.o,$(filter %.c %.cpp,$(SRC_FILES)))))

VPATH               := $(SRC_PATH) $(TEENSY3_COPY)
CC                  := $(abspath $(COMPILERPATH))/arm-none-eabi-gcc
CXX                 := $(abspath $(COMPILERPATH))/arm-none-eabi-g++
OBJCOPY             := $(abspath $(COMPILERPATH))/arm-none-eabi-objcopy
SIZE                := $(abspath $(COMPILERPATH))/arm-none-eabi-size
TEENSY_POST_COMPILE := $(abspath $(TOOLSPATH))/teensy_post_compile
TEENSY_REBOOT       := $(abspath $(TOOLSPATH))/teensy_reboot

all: build

.PHONY: clean clean-all
clean:
	@rm -rf $(OUT_PATH)

distclean: clean
	@rm -rf $(TEENSY3_COPY)

symbols: $(OUT_PATH)/eclipse_cdt_symbols.xml

ifeq  ($(wildcard $(TEENSY3_COPY)/.stamp),)
build: $(TEENSY3_COPY)/.stamp
upload: $(TEENSY3_COPY)/.stamp
$(OUT_PATH)/%.hex: $(TEENSY3_COPY)/.stamp
$(OUT_PATH)/$(M_PROJECT).elf: $(TEENSY3_COPY)/.stamp
else
build: $(OUT_PATH)/$(M_PROJECT).hex

upload: $(OUT_PATH)/$(M_PROJECT).hex
	@$(TEENSY_POST_COMPILE) -file=$(OUT_PATH)/$(M_PROJECT) -path=$(shell pwd) \
	  -tools=$(abspath $(TOOLSPATH))
	@echo "Rebooting Teensy"
	@-$(TEENSY_REBOOT)

$(OUT_PATH)/%.hex: $(OUT_PATH)/%.elf
	@$(SIZE) $<
	@$(OBJCOPY) -O ihex -R .eeprom $< $@

$(OUT_PATH)/$(M_PROJECT).elf: $(TEENSY3_COPY)/.stamp $(TEENSY3_OBJECTS) $(SRC_OBJECTS)
	@echo "Linking: "$@
	@$(CC) $(LDFLAGS) -o $@ $(TEENSY3_OBJECTS) $(SRC_OBJECTS) $(LIBS)
endif

$(TEENSY3_COPY)/.stamp: $(ALL_TEENSY3_FILES)
	@for f in $(TEENSY3_CORE_FILES); do mkdir -p `dirname $(TEENSY3_COPY)/$$f`; \
	  cp -pf $(TEENSY3_CORE_PATH)/$$f $(TEENSY3_COPY)/$$f; done
	@for f in $(TEENSY3_OVERRIDE_FILES); do mkdir -p `dirname $(TEENSY3_COPY)/$$f`; \
	  cp -pf $(TEENSY3_OVERRIDE_PATH)/$$f $(TEENSY3_COPY)/$$f; done
	@touch teensy3.copy/.stamp
	@$(MAKE) $(MAKECMDGOALS)

$(OUT_PATH):
	@mkdir -p $(OUT_PATH)


$(TEENSY3_OBJECTS): | $(OUT_PATH)

$(SRC_OBJECTS): | $(OUT_PATH)

$(OUT_PATH)/%.o: %.cpp
	@echo $<" -> "$@
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

$(OUT_PATH)/%.o: %.c
	@echo $<" -> "$@
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

-include $(TEENSY3_OBJECTS:.o=.d)
-include $(SRC_OBJECTS:.o=.d)

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
