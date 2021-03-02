# This is Makefile for ESP32 IDF

COMPONENT_ADD_INCLUDEDIRS := ./src
COMPONENT_SRCDIRS := ./src \
                     ./src/proto \
                     ./src/hal \
                     ./src/proto/hdlc/high_level \
                     ./src/proto/hdlc/low_level \
                     ./src/proto/fd \
                     ./src/proto/light \
                     ./src/proto/crc \

CPPFLAGS += \
            -DTINY_LOG_LEVEL_DEFAULT=0 \
            -DTINY_HDLC_DEBUG=0 \
            -DTINY_FD_DEBUG=0 \
            -DTINY_DEBUG=0

CPPFLAGS += \
            -DCONFIG_ENABLE_FCS32 \
            -DCONFIG_ENABLE_FCS16 \
            -DCONFIG_ENABLE_CHECKSUM \
            -DCONFIG_ENABLE_STATS
