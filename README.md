# Tiny Protocol

Tiny Microcontroller Communication Protocol.


1. Introduction

This protocol is intended to be used in low-memory systems, like
microcontrollers (Stellaris, Arduino). It is also can be compiled
for desktop Linux systems, and it can be built it for
Windows. All you need is to implement callback for writing and
reading bytes from communication line.

2. Resources required

Tiny Protocol has low memory consumption. The only memory needed to work is
a) Memory for Tiny Protocol state machine (24 bytes for Arduino Nano Atmega368p)
b) Memory to hold data being sent or received.

Tiny protocol has C-Style API, which is suitable for many low-resource systems.
Also, Arduino version has C++ API, which requires no additional memory.

On Desktop systems, which supports mutexes and threads, Tiny Protocol API becomes thread-safe,
allowing sending messages via the same channel from parallel threads.
