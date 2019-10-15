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

