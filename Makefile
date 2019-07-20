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
TINYCONF ?= normal

VERSION=0.7.0

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

.SUFFIXES: .c .cpp

$(BLD)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) -c $< -o $@

$(BLD)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -std=c++11 $(CCFLAGS) -c $< -o $@

# ************* Common defines ********************

INCLUDES += \
	-I./src \
        -I./src/proto \
        -I./tools/serial \

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
	unittest/helpers/tiny_hdlc_helper.cpp \
	unittest/helpers/tiny_light_helper.cpp \
	unittest/helpers/tiny_hd_helper.cpp \
	unittest/main.cpp \
	unittest/basic_tests.cpp \
	unittest/hdlc_tests.cpp \
	unittest/light_tests.cpp \
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
        src/proto/crc.c \
        src/proto/tiny_layer2.c \
        src/proto/tiny_light.c \
        src/proto/hdlc/tiny_hdlc.c \
        src/proto/tiny_hd.c \
        src/proto/tiny_list.c \
        src/proto/tiny_rq_pool.c \
        tools/serial/serial_linux.c

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

####################### all      ###################################

docs:
	doxygen doxygen.cfg

all: library sperf

install: install-lib
	$(STRIP) $(DESTDIR)/$@

clean:
	rm -rf $(BLD)

unittest: $(OBJ_UNIT_TEST) library
	$(CXX) $(CCFLAGS) -o $(BLD)/unit_test $(OBJ_UNIT_TEST) -L. -L$(BLD) -lm -pthread -ltinyp -lCppUTest -lCppUTestExt

sperf: $(OBJ_SPERF) library
	$(CC) $(CCFLAGS) -o $(BLD)/sperf $(OBJ_SPERF) -L. -L$(BLD) -lm -pthread -ltinyp

check: unittest
	$(BLD)/unit_test

release: docs
