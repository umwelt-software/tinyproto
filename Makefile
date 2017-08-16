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

VERSION=0.6.0

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
	$(CXX) -std=c++11 $(CCFLAGS) -c $< -o $@

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

.PHONY: clean library all install install-lib docs \
	arduino-pkg unittest check release

TARGET_UART = testuart

SRC_UNIT_TEST = \
	unittest/helpers/fake_wire.cpp \
	unittest/helpers/fake_channel.cpp \
	unittest/helpers/tiny_helper.cpp \
	unittest/helpers/tiny_hd_helper.cpp \
	unittest/main.cpp \
	unittest/basic_tests.cpp \
	unittest/hd_tests.cpp

OBJ_UNIT_TEST = $(addprefix $(BLD)/, $(addsuffix .o, $(basename $(SRC_UNIT_TEST))))

SRC_SPERF = \
        tools/sperf/sperf.c

OBJ_SPERF = $(addprefix $(BLD)/, $(addsuffix .o, $(basename $(SRC_SPERF))))

####################### Compiling library #########################

LIBS_TINY += -L. -lm

LDFLAGS_TINY = -shared

TARGET_TINY = libtinyp.a

SRC_TINY = \
        src/lib/crc.c \
        src/lib/tiny_layer2.c \
        src/lib/tiny_light.c \
        src/lib/tiny_hd.c \
        src/lib/tiny_list.c \
        src/lib/tiny_rq_pool.c \
        src/lib/serial/serial_linux.c

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

#####################################################################################################
####################### arduino library                                       #######################
#####################################################################################################

ARDUINO_BASE_LIB=TinyProtocol
ARDUINO_LIB=TinyProto

ARDUINO_BASE_DIR=./src/arduino
ARDUINO_DIR=./releases/arduino/$(ARDUINO_LIB)
ARDUINO_BASE_URL=https://github.com/lexus2k/tinyproto/tree/master/releases/arduino

arduino-pkg:
	@mkdir -p $(ARDUINO_DIR)
	@cp -rf -L $(ARDUINO_BASE_DIR)/* $(ARDUINO_DIR)/
	@mkdir -p $(ARDUINO_DIR)/src/proto
	@cp -rf -L ./src/lib/* $(ARDUINO_DIR)/src/proto/
	@cp -rf -L ./tools $(ARDUINO_DIR)/
	@cp -f -L ./inc/*.h $(ARDUINO_DIR)/src/proto/
	@cp -f -L ./inc/serial_api.h $(ARDUINO_DIR)/src/proto/serial/
	@cp -f -L ./inc/os/arduino/*.h $(ARDUINO_DIR)/src/proto/
	@mv $(ARDUINO_DIR)/library.properties.in $(ARDUINO_DIR)/library.properties
	@sed -i "s/VERSION/$(VERSION)/" $(ARDUINO_DIR)/library.properties
	@sed -i "s/LIBRARY/$(ARDUINO_LIB)/" $(ARDUINO_DIR)/library.properties
	@sed -i "s,ADDRESS,$(ARDUINO_BASE_URL)/$(ARDUINO_LIB),g" $(ARDUINO_DIR)/library.properties
	@echo "arduino package build ... [DONE]"

####################### ARDUINO LIB TEST ###########################

SRC_TEST_ARDUINO_LIB = \
        $(ARDUINO_DIR)/src/proto/crc.c \
        $(ARDUINO_DIR)/src/proto/tiny_layer2.c \
	$(ARDUINO_DIR)/src/TinyProtocol.cpp \
	$(ARDUINO_DIR)/src/TinyProtocolHd.cpp \
#        src/lib/tiny_request_pool.c \
#        src/lib/tiny_list.c \

OBJ_TEST_ARDUINO_LIB = $(addprefix $(BLD)/, $(addsuffix .o, $(basename $(SRC_TEST_ARDUINO_LIB))))

$(OBJ_TEST_ARDUINO_LIB): arduino-pkg

test-arduino-lib: $(OBJ_TEST_ARDUINO_LIB)



####################### all      ###################################

docs:
	doxygen doxygen.cfg

all: library docs unittest sperf

install: install-lib
	$(STRIP) $(DESTDIR)/$@

clean:
	rm -rf $(BLD)
	rm -rf docs
	rm -rf releases

unittest: $(OBJ_UNIT_TEST) library
	$(CXX) $(CCFLAGS) -o $(BLD)/unit_test $(OBJ_UNIT_TEST) -L. -L$(BLD) -lm -pthread -ltinyp -lCppUTest -lCppUTestExt

sperf: $(OBJ_SPERF) library
	$(CC) $(CCFLAGS) -o $(BLD)/sperf $(OBJ_SPERF) -L. -L$(BLD) -lm -pthread -ltinyp

check: unittest
	$(BLD)/unit_test

release: arduino-pkg docs
