/*
    Copyright 2017 (C) Alexey Dynda

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
*/

#ifndef _SERIAL_API_H_
#define _SERIAL_API_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

typedef void * SerialHandle;

#ifdef _WIN32

#include <windows.h>

#define INVALID_SERIAL  INVALID_HANDLE_VALUE
#define SERIAL_115200   CBR_115200
#define SERIAL_38400    CBR_38400
#define SERIAL_9600     CBR_9600

#elif __linux__

#include <termios.h>
#include <stdlib.h>
#include <string.h>
#define INVALID_SERIAL  (SerialHandle)-1
#define SERIAL_115200   B115200
#define SERIAL_38400    B38400
#define SERIAL_9600     B9600

#elif ARDUINO

#else

#endif

extern SerialHandle OpenSerial(const char* name, uint32_t baud);

extern void CloseSerial(SerialHandle port);

extern int SerialSend(SerialHandle hPort, const uint8_t *buf, int len);

extern int SerialReceive(SerialHandle hPort, uint8_t *buf, int len);

#ifdef __cplusplus
}
#endif

#endif
