#################################################################
# Makefile to build tiny protocol library
#
# Accept the following variables
# CROSS_COMPILE
# CC
# CXX
# STRIP
# AR

default: all
SDK_BASEDIR ?=
CROSS_COMPILE ?=
OS ?= os/linux
DESTDIR ?=
BLD ?= bld

VERSION=0.1.0

ifeq ($(TINYCONF), nano)
    CONFIG_ENABLE_FCS32 ?= n
    CONFIG_ENABLE_FCS16 ?= n
    CONFIG_ENABLE_CHECKSUM ?= n
    CONFIG_ENABLE_STATS ?= n
else ifeq ($(TINYCONF), normal)
    CONFIG_ENABLE_FCS32 ?= n
    CONFIG_ENABLE_FCS16 ?= y
    CONFIG_ENABLE_CHECKSUM ?= y
    CONFIG_ENABLE_STATS ?= n
else
    CONFIG_ENABLE_FCS32 ?= y
    CONFIG_ENABLE_FCS16 ?= y
    CONFIG_ENABLE_CHECKSUM ?= y
    CONFIG_ENABLE_STATS ?= y
endif

# set up compiler and options
ifeq ($(CROSS_COMPILE),)
    STRIP ?= strip
    AR ?= ar
else
    CC = $(CROSS_COMPILE)gcc
    CXX = $(CROSS_COMPILE)g++
    STRIP = $(CROSS_COMPILE)strip
    AR = $(CROSS_COMPILE)ar
endif

export SDK_BASEDIR
export CROSS_COMPILE

.SUFFIXES: .c .cpp

$(BLD)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) -c $< -o $@

$(BLD)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CCFLAGS) -c $< -o $@

#.cpp.o:
#	mkdir -p $(BLD)/$(dir $@)
#	$(CXX) $(CCFLAGS) -c $< -o $@
# ************* Common defines ********************

INCLUDES += \
        -I./inc \
        -I$(SDK_BASEDIR)/usr/include \
        -I./inc/$(OS)/ \
        -I$(OS)/include

CCFLAGS += -fPIC -g $(INCLUDES) -Wall -Werror

ifeq ($(CONFIG_ENABLE_FCS32),y)
    CCFLAGS += -DCONFIG_ENABLE_FCS32
endif

ifeq ($(CONFIG_ENABLE_FCS16),y)
    CCFLAGS += -DCONFIG_ENABLE_FCS16
endif

ifeq ($(CONFIG_ENABLE_CHECKSUM),y)
    CCFLAGS += -DCONFIG_ENABLE_CHECKSUM
endif

ifeq ($(CONFIG_ENABLE_STATS),y)
    CCFLAGS += -DCONFIG_ENABLE_STATS
endif

.PHONY: clean library examples all install install-lib doc \
	arduino-nano arduino-micro arduino-full arduino-pkg

# ************* Compiling example ******************

LIBS_EXAMPLE += -L. -L$(BLD) -lm -lpthread -ltinyp

TARGET_EXAMPLE = testproto

TARGET_UART = testuart

SRC_EXAMPLE = \
        $(OS)/src/uart.c \
        $(OS)/src/fakewire.c \
        src/examples/testproto.c

OBJ_EXAMPLE = $(addprefix $(BLD)/, $(addsuffix .o, $(basename $(SRC_EXAMPLE))))

SRC_UART_TEST = \
        $(OS)/src/uart.c \
        src/examples/testuart.c

OBJ_UART = $(addprefix $(BLD)/, $(addsuffix .o, $(basename $(SRC_UART_TEST))))

####################### Compiling library #########################

LIBS_TINY += -L. -lm

LDFLAGS_TINY = -shared

TARGET_TINY = libtinyp.a

SRC_TINY = \
        src/lib/crc.c \
        src/lib/tiny_layer2.c \

OBJ_TINY = $(addprefix $(BLD)/, $(addsuffix .o, $(basename $(SRC_TINY))))


library: $(OBJ_TINY)
	@echo SDK_BASEDIR: $(SDK_BASEDIR)
	@echo CROSS_COMPILE: $(CROSS_COMPILE)
	@echo OS: $(OS)
	$(AR) rcs $(BLD)/$(TARGET_TINY) $(OBJ_TINY)
#	$(STRIP) $(BLD)/$(TARGET_TINY)
#	$(CC) $(CCFLAGS) -fPIC -o $(BLD)/$(TARGET_TINY).so $(OBJ_TINY) $(LIBS_TINY) $(LDFLAGS_TINY)

install-lib: library
	cp -r $(BLD)/libtinyp.a $(DESTDIR)/usr/lib/
	cp -rf ./include/*.h $(DESTDIR)/usr/include/
	cp -rf ./include/$(OS)/*.h $(DESTDIR)/usr/include/

####################### Examples ###################################

examples: $(OBJ_EXAMPLE) $(OBJ_UART) library
	$(CC) $(CCFLAGS) -o $(BLD)/$(TARGET_EXAMPLE) $(OBJ_EXAMPLE) $(LIBS_EXAMPLE)
	$(CC) $(CCFLAGS) -o $(BLD)/$(TARGET_UART) $(OBJ_UART) $(LIBS_EXAMPLE)

#####################################################################################################
####################### arduino libraries                                     #######################
#####################################################################################################

ARDUINO_BASE_LIB=TinyProtocol
ARDUINO_NANO_LIB=TinyProto-Nano
ARDUINO_MICRO_LIB=TinyProto-Micro
ARDUINO_FULL_LIB=TinyProto-Full

ARDUINO_BASE_DIR=./src/arduino
ARDUINO_NANO_DIR=./releases/arduino/$(ARDUINO_NANO_LIB)
ARDUINO_MICRO_DIR=./releases/arduino/$(ARDUINO_MICRO_LIB)
ARDUINO_FULL_DIR=./releases/arduino/$(ARDUINO_FULL_LIB)


arduino-nano:
	@mkdir -p $(ARDUINO_NANO_DIR)
	@cp -rf -L $(ARDUINO_BASE_DIR)/* $(ARDUINO_NANO_DIR)/
	@echo "#define CONFIG_ENABLE_CHECKSUM" > $(ARDUINO_NANO_DIR)/src/proto/tiny_config.h
	@sed -i "s/VERSION/$(VERSION)/" $(ARDUINO_NANO_DIR)/library.properties
	@sed -i "s/LIBRARY/$(ARDUINO_NANO_LIB)/" $(ARDUINO_NANO_DIR)/library.properties
	@mv $(ARDUINO_NANO_DIR)/src/$(ARDUINO_BASE_LIB).h $(ARDUINO_NANO_DIR)/src/$(ARDUINO_NANO_LIB).h
	@sed -i "s/$(ARDUINO_BASE_LIB).h/$(ARDUINO_NANO_LIB).h/" $(ARDUINO_NANO_DIR)/src/TinyProtocol.cpp

arduino-micro:
	@mkdir -p $(ARDUINO_MICRO_DIR)/
	@cp -rf -L $(ARDUINO_BASE_DIR)/* $(ARDUINO_MICRO_DIR)/
	@echo "#define CONFIG_ENABLE_CHECKSUM" > $(ARDUINO_MICRO_DIR)/src/proto/tiny_config.h
	@echo "#define CONFIG_ENABLE_FCS16" >> $(ARDUINO_MICRO_DIR)/src/proto/tiny_config.h
	@sed -i "s/VERSION/$(VERSION)/" $(ARDUINO_MICRO_DIR)/library.properties
	@sed -i "s/LIBRARY/$(ARDUINO_MICRO_LIB)/" $(ARDUINO_MICRO_DIR)/library.properties
	@mv $(ARDUINO_MICRO_DIR)/src/$(ARDUINO_BASE_LIB).h $(ARDUINO_MICRO_DIR)/src/$(ARDUINO_MICRO_LIB).h
	@sed -i "s/$(ARDUINO_BASE_LIB).h/$(ARDUINO_MICRO_LIB).h/" $(ARDUINO_MICRO_DIR)/src/TinyProtocol.cpp

arduino-full:
	@mkdir -p $(ARDUINO_FULL_DIR)/
	@cp -rf -L $(ARDUINO_BASE_DIR)/* $(ARDUINO_FULL_DIR)/
	@echo "#define CONFIG_ENABLE_CHECKSUM" > $(ARDUINO_FULL_DIR)/src/proto/tiny_config.h
	@echo "#define CONFIG_ENABLE_FCS16" >> $(ARDUINO_FULL_DIR)/src/proto/tiny_config.h
	@echo "#define CONFIG_ENABLE_FCS32" >> $(ARDUINO_FULL_DIR)/src/proto/tiny_config.h
	@echo "#define CONFIG_ENABLE_STATS" >> $(ARDUINO_FULL_DIR)/src/proto/tiny_config.h
	@sed -i "s/VERSION/$(VERSION)/" $(ARDUINO_FULL_DIR)/library.properties
	@sed -i "s/LIBRARY/$(ARDUINO_FULL_LIB)/" $(ARDUINO_FULL_DIR)/library.properties
	@mv $(ARDUINO_FULL_DIR)/src/$(ARDUINO_BASE_LIB).h $(ARDUINO_FULL_DIR)/src/$(ARDUINO_FULL_LIB).h
	@sed -i "s/$(ARDUINO_BASE_LIB).h/$(ARDUINO_FULL_LIB).h/" $(ARDUINO_FULL_DIR)/src/TinyProtocol.cpp

arduino-pkg: arduino-nano arduino-micro arduino-full
	@echo "arduino package build ... [DONE]"


####################### all      ###################################

doc:
	doxygen doxygen.cfg

all: examples library doc

install: install-lib
	$(STRIP) $(DESTDIR)/$@

clean:
	rm -rf $(BLD)
	rm -rf doc
	rm -rf releases

