ARCH ?= linux

include Makefile.$(ARCH)

.PHONY: help

help:
	@echo "Usage make [options] targets"
	@echo "    options:"
	@echo "        DESTDIR=path                  Specify install destination"
	@echo "        ARCH=<platform>               Specify platform: linux, mingw32, avr, esp32"
	@echo "        CONFIG_ENABLE_FCS32=<y/n>     Enable or disable FCS32 support"
	@echo "        CONFIG_ENABLE_FCS16=<y/n>     Enable or disable FCS16 support"
	@echo "        CONFIG_ENABLE_CHECKSUM=<y/n>  Enable or disable checksum support"
	@echo "        EXAMPLES=<y/n>                Build examples"
	@echo "        CUSTOM=<y/n>                  Do not build built-in HAL, use custom HAL implementation"
	@echo "    debug options:"
	@echo "        LOG_LEVEL=<1-5>               Set log level, 5 - max logs"
	@echo "        ENABLE_LOGS=<y/n>             Enable or disable logging"
	@echo "        ENABLE_HDLC_LOGS=<y/n>        Enable or disable hdlc low/high level logs"
	@echo "        ENABLE_FD_LOGS=<y/n>          Enable or disable full duplex protocol logs"
	@echo "    targets:"
	@echo "        library        Build library"
	@echo "        cppcheck       Run cppcheck tool for code static verification"
	@echo "        check          Build and run unit tests"
	@echo "        unittest       Only build unit tests"
	@echo "        all            Build tinyproto library and tiny_loopback tool"
	@echo "        tiny_loopback  Build tiny_loopback tool"
	@echo "        docs           Build library documentation (requires doxygen)"
	@echo "        install        Install library"
	@echo "        clean          Remove all temporary generated files and binaries"
