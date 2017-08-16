# Tiny Protocol

Tiny Microcontroller Communication Protocol.

1. Introduction

Tiny Protocol is layer 2 simple protocol. It is intended to be used in low-memory systems,
like microcontrollers (Stellaris, Arduino). It is also can be compiled
for desktop Linux systems, and it can be built it for
Windows. All you need is to implement callback for writing and
reading bytes from communication line.

2. Resources required

Tiny Protocol has low memory consumption. The only memory needed to work is
a) Memory for Tiny Protocol state machine (36 bytes for Arduino Nano Atmega368p)
b) Memory to hold data being sent or received.

Tiny protocol has C-Style API, which is suitable for many low-resource systems.
Also, Arduino version has C++ API, which requires no additional memory.

On Desktop systems, which supports mutexes and threads, Tiny Protocol API can be used
for sending from parallel threads. Please, not that read operations are not thread-safe.

3. Features

 * Simple 8-bit checksum to control data validity
 * FCS16 (CCITT-16) to control data validity
 * FCS32 (CCITT-32) to control data validity
 * Using uids with user data
 * Sending frames with acknowledgement
 * Sending and receiving the frames of maximum 64K size.
 * Low memory consumptions (36 bytes for Tiny Protocol engine)
 * Tool for communicating with uC devices (with win32 binary)

For more information, please refer to [Documentation](http://lexus2k.github.io/tinyproto).

Please refer to [Arduino version](https://github.com/lexus2k/arduino-protocol) if you need to add library to Arduino IDE.
