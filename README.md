![Tiny Protocol](.travis/tinylogo.svg)<br>
[![Build Status](https://travis-ci.org/lexus2k/tinyproto.svg?branch=master)](https://travis-ci.org/lexus2k/tinyproto)
[![Coverage Status](https://coveralls.io/repos/github/lexus2k/tinyproto/badge.svg?branch=master)](https://coveralls.io/github/lexus2k/tinyproto?branch=master)

[tocstart]: # (toc start)

  * [Introduction](#introduction)
  * [Key Features](#key-features)
  * [Supported platforms](#supported-platforms)
  * [Easy to use](#easy-to-use)
  * [Setting up](#setting-up)
  * [Using tiny_loopback tool](#using-tiny_loopback-tool)
  * [License](#license)

[tocend]: # (toc end)

## Introduction

Tiny Protocol is layer 2 protocol. It is intended to be used for the systems with
small amount of resources. It is also can be compiled for desktop Linux system, and it can
be built it for Windows. All you need is to implement callback for writing and
reading bytes from communication line, and implement 2 application callback for incoming
message event and outgoing message event. With this library you can easy communicate your Arduino with applications
on PC and other boards. You don't need to think about data synchronization
between points. No dynamic allocation of memory, so, it can be used on the systems with limited resources

## Key Features

Main features:
 * 4 protocol variants depending on your needs (basic hdlc, light, hd and fd)
 * Error detection (basic hdlc, hd and fd variants)
   * Simple 8-bit checksum (sum of bytes)
   * FCS16 (CCITT-16)
   * FCS32 (CCITT-32)
 * Frame transfer acknowledgement (hd and fd variants)
 * Frames of maximum 32K or 2G size (limit depends on platform).
 * Low SRAM consumption.
 * Low Flash consumption (features can be disabled and enabled at compilation time)
 * No dynamic memory allocations (suitable for using on uC with limited resources or without memory manager)
 * Zero copy implementation (basic hdlc, light versions do not use copy operations)
 * Serial loopback tool for debug purposes and performance testing

## Supported platforms

 * Any platform, where C/C++ compiler is available (C99, C++11)

## Easy to use

Using light variant of Tiny Protocol can look like this:
```.cpp
Tiny::ProtoLight  proto;
Tiny::Packet<256> packet;
...
    if (Serial.available()) {
        int len = proto.read( packet );
        if (len > 0) {
            /* Send message back */
            proto.write( packet );
        }
    }
```

Example of using fd variant of Tiny Protocol is a little bit bigger, but it is still simple:
```.cpp
Tiny::ProtoFd<FD_MIN_BUF_SIZE(64,4)>  proto;

void onReceive(Tiny::IPacket &pkt) {
    if ( proto.write(pkt) == TINY_ERR_TIMEOUT ) {
        // Do what you need to do if looping back failed on timeout.
        // But never use blocking operations inside callback
    }
}
...
proto.setReceiveCallback( onReceive );
...
void loop() {
    if (Serial.available()) {
        proto.run_rx();
    }
    proto.run_tx();
}
```

## Setting up

 * Arduino Option 1 (with docs and tools)
   * Download source from https://github.com/lexus2k/tinyproto
   * Put the downloaded library content to Arduino/libraries/tinyproto folder
   * Restart the Arduino IDE
   * You will find the examples in the Arduino IDE under File->Examples->tinyproto

 * Arduino Option 2 (only library without docs)
   * Go to Arduino Library manager
   * Find and install tinyproto library
   * Restart the Arduino IDE
   * You will find the examples in the Arduino IDE under File->Examples->tinyproto

 * ESP32 IDF
   * Download sources from https://github.com/lexus2k/tinyproto and put to components
     folder of your project
   * Run `make` for your project

 * Linux
   * Download sources from https://github.com/lexus2k/tinyproto
   * Run `make` command from tinyproto folder, and it will build library and tools for you

 * Plain AVR
   * Download sources from https://github.com/lexus2k/tinyproto
   * Install avr gcc compilers
   * Run `make ARCH=avr`

## Using tiny_loopback tool

 * Connect your Arduino board to PC
 * Run your sketch or tinylight_loopback
 * Compile tiny_loopback tool
 * Run tiny_loopback tool: `./bld/tiny_loopback -p /dev/ttyUSB0 -t light -g -c 8 -a -r`

 * Connect your Arduino board to PC
 * Run your sketch or tinyhd_loopback
 * Compile tiny_loopback tool
 * Run tiny_loopback tool: `./bld/tiny_loopback -p /dev/ttyUSB0 -t hd -c 8 -g -a -r`

 * Connect your Arduino board to PC
 * Run your sketch or tinyfd_loopback
 * Compile tiny_loopback tool
 * Run tiny_loopback tool: `./bld/tiny_loopback -p /dev/ttyUSB0 -t fd -c 8 -w 3 -g -a -r`

For more information about this library, please, visit https://github.com/lexus2k/tinyproto.
Doxygen documentation can be found at [github.io site](http://lexus2k.github.io/tinyproto).
If you found any problem or have any idea, please, report to Issues section.

## License

Copyright 2016-2019 (C) Alexey Dynda

This file is part of Tiny Protocol Library.

Protocol Library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Protocol Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.

