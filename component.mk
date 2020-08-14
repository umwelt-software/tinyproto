# This is Makefile for ESP32 IDF

COMPONENT_ADD_INCLUDEDIRS := ./src
COMPONENT_SRCDIRS := ./src \
                     ./src/proto \
                     ./src/proto/hal \
                     ./src/proto/hdlc \
                     ./src/proto/fd \
                     ./src/proto/half_duplex \
                     ./src/proto/light \
                     ./src/proto/crc \

CPPFLAGS += \
            -DTINY_LOG_LEVEL_DEFAULT=0 \
            -DTINY_HDLC_DEBUG=0 \
            -DTINY_HD_DEBUG=0 \
            -DTINY_FD_DEBUG=0 \
            -DTINY_DEBUG=0

CPPFLAGS += \
            -DCONFIG_ENABLE_FCS32 \
            -DCONFIG_ENABLE_FCS16 \
            -DCONFIG_ENABLE_CHECKSUM \
            -DCONFIG_ENABLE_STATS
