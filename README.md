# Tiny Protocol

## Introduction

Tiny Protocol is layer 2 simple protocol. It is intended to be used for the systems with
small amount of resources. It is also can be compiled for desktop Linux system, and it can
be built it for Windows. All you need is to implement callback for writing and
reading bytes from communication line.
With this library you can easy communicate your Arduino with applications
on PC and other boards. You don't need to think about data synchronization
between points.

## History

To implement communication with synchronization between two microcontrollers, I needed some
protocol. Passing raw bytes over communication channel has big disadvantages: absence of 
synchronization, no error correction, that means if some device is connected to another 
one at any time point, there is a big chance to get some garbage on communication line.
Thus, the first version of TinyProto appeared. It supports error checking and framing for
the data being sent. But very soon I realized, that in many cases the communication channels
are stable enough, but having error detection requires a lot of place for code in uC flash.
And then TinyProto Light version was implemented.

## Features

 * Error detection (absent in TinyProto Light version)
   * Simple 8-bit checksum
   * FCS16 (CCITT-16)
   * FCS32 (CCITT-32)
 * Passing UID with each frame (absent in TinyProto Light version)
 * Frame transfer acknowledgement (TinyProto Half Duplex version)
 * Frames of maximum 64K size.
 * Low SRAM consumption (36 bytes for Tiny Protocol engine)
 * Low Flash consumption (many features can be disabled at compilation time)
 * Tool for communicating with uC devices (with win32 binary)

## Supported platforms

 * Any platform, where C/C++ compiler is available (C99, C++11)

## TinyProto available implementations

 * TinyProtocol Light (refer to [tiny_light.h](inc/tiny_light.h))
   * Data framing
   * No error detection
   * Only blocking operations
   * No send confirmation
 * TinyProtocol (refer to [tiny_layer2.h](inc/tiny_layer2.h))
   * Simple (no uids, blocking operations) and Full version
   * Passing uid with the frame (optional)
   * Error correction support (checksum, FCS16, FCS32)
   * Blocking and non-blocking APIs
   * No send confirmation
 * TinyProtocol Half Duplex (refer to [tiny_hd.h](inc/tiny_hd.h))
   * Based on top of TinyProtocol
   * Additionally to TinyProtocol supports frame acknowledgements

## Setting up for Arduino

 * Download source from https://github.com/lexus2k/tinyproto
 * Put the /tinyproto/releases/arduino/ folder content  to Arduino/libraries/ folder

## Running demo example for Arduino

 * Connect your Arduino board to PC
 * Burn and run tinylight_loopback sketch on Arduino board
 * Run sperf tool on the PC

sperf sends frames to Arduino board, and tinylight_loopback sketch sends all frames back to PC.


For more information about this library, please, visit https://github.com/lexus2k/tinyproto.
Doxygen documentation can be found at [github.io site](http://lexus2k.github.io/tinyproto).
If you found any problem or have any idea, please, report to Issues section.

## License

Copyright 2016-2018 (C) Alexey Dynda

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

