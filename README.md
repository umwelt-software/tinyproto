# Tiny Protocol

[tocstart]: # (toc start)

  * [Introduction](#introduction)
  * [Key Features](#key-features)
  * [Supported platforms](#supported-platforms)
  * [Setting up](#setting-up)
  * [Using hd_loopback tool](#using-hd_loopback-tool)
  * [Running sperf](#running-sperf)
  * [License](#license)

[tocend]: # (toc end)

## Introduction

Tiny Protocol is layer 2 simple protocol. It is intended to be used for the systems with
small amount of resources. It is also can be compiled for desktop Linux system, and it can
be built it for Windows. All you need is to implement callback for writing and
reading bytes from communication line.
With this library you can easy communicate your Arduino with applications
on PC and other boards. You don't need to think about data synchronization
between points.
No dynamic allocation of memory, so, it can be used on the systems with limited resources

## Key Features

This protocol allows to organize communication either between PC and EVK board or
between two microcontrollers. Passing raw bytes over communication channel has big
disadvantages:
 * absence of synchronization. If devices start at different time, then some device can start
   reading from the middle of the frame data,
 * no error correction. If hardware channel is noise, there is a big chance to get some garbage
   on communication line.
These issues can be solved with tinyproto protocol. It supports error checking and framing for
the data being sent.

Main features:
 * Error detection (absent in TinyProto Light version)
   * Simple 8-bit checksum
   * FCS16 (CCITT-16)
   * FCS32 (CCITT-32)
 * Frame transfer acknowledgement (TinyProto Half Duplex version)
 * Frames of maximum 32K size (limit depends on OS).
 * Low SRAM consumption
 * Low Flash consumption (many features can be disabled at compilation time)
 * Serial performance tool (sperf)
 * Loopback tool for debug purposes
 * No dynamic memory allocations (suitable for using on uC with limited resources)
 * Zero copy implementation (application buffers are used as is, no copy operations)

## Supported platforms

 * Any platform, where C/C++ compiler is available (C99, C++11)

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

## Using hd_loopback tool

 * Connect your Arduino board to PC
 * Run your sketch or sketch_hd_sender
 * Compile hd_loopback tool
 * Run hd_loopback tool: `./bld/hd_loopback -p /dev/ttyUSB0 -c 8 -g`

## Running sperf

 * Connect your Arduino board to PC
 * Burn and run tinylight_loopback sketch on Arduino board
 * Run sperf tool on the PC

sperf sends frames to Arduino board, and tinylight_loopback sketch sends all frames back to PC.

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

