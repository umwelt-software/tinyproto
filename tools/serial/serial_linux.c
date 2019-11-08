/*
    Copyright 2017-2019 (C) Alexey Dynda

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

#include "serial_api.h"

#ifdef __linux__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <linux/serial.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>

#define DEBUG_SERIAL 0
#define DEBUG_SERIAL_TX DEBUG_SERIAL
#define DEBUG_SERIAL_RX DEBUG_SERIAL

static int handleToFile(SerialHandle handle)
{
    return (int)handle;
}

static SerialHandle fileToHandle(int fd)
{
    SerialHandle handle = 0;
    handle = (SerialHandle)fd;
    return handle;
}

void CloseSerial(SerialHandle port)
{
    if (handleToFile(port) >= 0)
    {
        close( handleToFile(port) );
    }
}


SerialHandle OpenSerial(const char* name, uint32_t baud)
{
    struct termios options;
    struct termios oldt;

    int fd = open( name, O_RDWR | O_NOCTTY | O_NONBLOCK );
    if (fd == -1)
    {
        perror("ERROR: Failed to open serial device");
        return INVALID_SERIAL;
    }
    fcntl(fd, F_SETFL, O_RDWR);

    if (tcgetattr(fd, &oldt) == -1)
    {
        close(fd);
        return INVALID_SERIAL;
    }
    options = oldt;
    cfmakeraw(&options);

    options.c_lflag &= ~ICANON;
    options.c_lflag &= ~(ECHO | ECHOCTL | ECHONL);
    options.c_cflag |= HUPCL;

    options.c_oflag &= ~ONLCR; /* set NO CR/NL mapping on output */
    options.c_iflag &= ~ICRNL; /* set NO CR/NL mapping on input */

    // no flow control
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    if (cfsetspeed(&options, baud) == -1)
    {
        close(fd);
        return INVALID_SERIAL;
    }
    if (cfsetospeed(&options, baud) == -1)
    {
        close(fd);
        return INVALID_SERIAL;
    }
    if (cfsetispeed(&options, baud) == -1)
    {
        close(fd);
        return INVALID_SERIAL;
    }

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;

    // Set the new options for the port...
    if (tcsetattr(fd, TCSAFLUSH, &options) == -1)
    {
        close(fd);
        return INVALID_SERIAL;
    }

    // Flush any buffered characters
    tcflush(fd, TCIOFLUSH);

    return fileToHandle(fd);
}

int SerialSend(SerialHandle hPort, const void *buf, int len)
{
    int ret;
    struct pollfd fds = {
           .fd = handleToFile(hPort),
           .events = POLLOUT | POLLWRNORM
    };
write_poll:
    ret = poll(&fds, 1, 100);
    if (ret < 0)
    {
        if ( errno == EINTR )
        {
            goto write_poll;
        }
        return ret;
    }
    if (ret == 0 || !(fds.revents & (POLLOUT | POLLWRNORM)))
    {
        return 0;
    }
    ret = write(handleToFile(hPort), buf, len);
    if ((ret < 0) && (errno == EAGAIN || errno == EINTR))
    {
        return 0;
    }
    if (ret > 0)
    {
#if DEBUG_SERIAL_TX == 1
        struct timespec s;
        clock_gettime( CLOCK_MONOTONIC, &s );
        for (int i=0; i<ret; i++) printf("%08llu: TX: 0x%02X '%c'\n",
                                         s.tv_nsec / 1000000ULL + s.tv_sec * 1000ULL,
                                         (uint8_t)(((const char *)buf)[i]),
                                         ((const char *)buf)[i]);
#endif
//        tcflush(handleToFile(hPort), TCOFLUSH);
    }
    return ret;
}


int SerialReceive(SerialHandle hPort, void *buf, int len)
{
    struct pollfd fds = {
           .fd = handleToFile(hPort),
           .events = POLLIN | POLLRDNORM
    };
    int ret;
read_poll:
    ret = poll(&fds, 1, 100);
    if (ret < 0)
    {
        if ( errno == EINTR )
        {
            goto read_poll;
        }
        return ret;
    }
    if (ret == 0 || !(fds.revents & (POLLIN | POLLRDNORM)))
    {
        return 0;
    }
    ret = read(handleToFile(hPort), buf, len);
    if ((ret < 0) && (errno == EAGAIN || errno == EINTR))
    {
        return 0;
    }
    if (ret > 0)
    {
#if DEBUG_SERIAL_RX == 1
        struct timespec s;
        clock_gettime( CLOCK_MONOTONIC, &s );
        for (int i=0; i<ret; i++) printf("%08llu: RX: 0x%02X '%c'\n",
                                         s.tv_nsec / 1000000ULL + s.tv_sec * 1000ULL,
                                         (uint8_t)(((char *)buf)[i]),
                                         ((char *)buf)[i]);
#endif
    }
    return ret;
}

#endif
