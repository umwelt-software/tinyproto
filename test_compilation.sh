#!/bin/sh


test_build()
{
    make clean
    CCFLAGS="$6" make CONFIG_ENABLE_FCS16=$1 CONFIG_ENABLE_FCS32=$2 CONFIG_ENABLE_CHECKSUM=$3 OS=$4 $5
    if [ $? -ne 0 ]; then
        echo "[FAILED] $1 $2 $3 $4"
        exit 1
    fi
}

test_build_conf()
{
    make clean
    CCFLAGS="$4" make TINYCONF=$1 OS=$2 $3
    if [ $? -ne 0 ]; then
        echo "[FAILED] $1 $2 $3 $4"
        exit 1
    fi
}

test_linux_lib()
{
    test_build n n n os/linux library
    test_build n y n os/linux library
    test_build y n n os/linux library
    test_build y y y os/linux library
    test_build n n y os/linux library
    test_build_conf full os/linux library
    test_build_conf minimal os/linux library
}

test_arduino_lib()
{
    test_build y y y os/arduino test-arduino-lib
    test_build n n n os/arduino library -DARDUINO
    test_build n y n os/arduino library
    test_build y n n os/arduino library
    test_build y y y os/arduino library
    test_build n n y os/arduino library
    test_build_conf nano os/arduino library -DARDUINO
    test_build_conf regular os/arduino library -DARDUINO
    test_build_conf full os/arduino library -DARDUINO
}

test_linux_app()
{
    test_build n n n os/linux all
    test_build y y y os/linux all
}


test_linux_lib
test_arduino_lib

test_linux_app

make clean
