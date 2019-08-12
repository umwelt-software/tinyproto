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
/**
 This is UART support implementation.

 @file
 @brief UART serial API
*/


#ifndef _SERIAL_API_H_
#define _SERIAL_API_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

/**
 * SerialHandle cross-platform type
 */
typedef uintptr_t SerialHandle;

#ifdef _WIN32

#include <windows.h>

/** Invalid serial handle definition */
#define INVALID_SERIAL  (uintptr_t)INVALID_HANDLE_VALUE
/** 115200 baud */
#define SERIAL_115200   CBR_115200
/** 38400 baud */
#define SERIAL_38400    CBR_38400
/** 9600 baud */
#define SERIAL_9600     CBR_9600

#elif __linux__

#include <termios.h>
#include <stdlib.h>
#include <string.h>

/** Invalid serial handle definition */
#define INVALID_SERIAL  ((SerialHandle)-1)
/** 115200 baud */
#define SERIAL_115200   B115200
/** 38400 baud */
#define SERIAL_38400    B38400
/** 9600 baud */
#define SERIAL_9600     B9600

#elif ARDUINO

#else

#endif

/**
 * @brief Opens serial port
 *
 * Opens serial port by name
 *
 * @param name - path to the port to open
 *               For linux this can be /dev/ttyO1, /dev/ttyS1, /dev/ttyUSB0, etc.
 *               For windows this can be COM1, COM2, etc.
 * @param baud - cross-platform baud constant, see SERIAL_115200
 * @return valid serial handle or INVALID_SERIAL in case of error
 * @see SERIAL_115200
 * @see SERIAL_38400
 * @see SERIAL_9600
 */
extern SerialHandle OpenSerial(const char* name, uint32_t baud);

/**
 * @brief Closes serial connection
 *
 * Closes serial connection
 * @param port - SerialHandle
 */
extern void CloseSerial(SerialHandle port);

/**
 * @brief Sends data over serial connection
 *
 * Sends data over serial connection.
 * @param port - SerialHandle
 * @param buf - pointer to data buffer to send
 * @param len - length of data to send
 * @return negative value in case of error.
 *         or number of bytes sent
 */
extern int SerialSend(SerialHandle hPort, const void *buf, int len);

/**
 * @brief Receive data from serial connection
 *
 * Receive data from serial connection.
 * @param port - SerialHandle
 * @param buf - pointer to data buffer to read to
 * @param len - maximum size of receive buffer
 * @return negative value in case of error.
 *         or number of bytes received
 */
extern int SerialReceive(SerialHandle hPort, void *buf, int len);

#ifdef __cplusplus
}
#endif

#endif
