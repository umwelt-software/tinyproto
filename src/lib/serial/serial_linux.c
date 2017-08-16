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


static int handleToFile(SerialHandle handle)
{
    return *((int *)&handle);
}

static SerialHandle fileToHandle(int fd)
{
    SerialHandle handle = 0;
    *((int *)&handle) = fd;
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

    int fd = open( name, O_RDWR | O_NOCTTY );
    if (fd == -1)
    {
        perror("ERROR: Failed to open serial device");
        return INVALID_SERIAL;
    }

    if (tcgetattr(fd, &options) == -1)
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

    cfmakeraw(&options);
    options.c_cflag &= ~CRTSCTS;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;

    // Flush any buffered characters
    tcflush(fd, TCIOFLUSH);

    // Set the new options for the port...
    if (tcsetattr(fd, TCSANOW, &options) == -1)
    {
        close(fd);
        return INVALID_SERIAL;
    }

    return fileToHandle(fd);
}

int SerialSend(SerialHandle hPort, const uint8_t *buf, int len)
{
    int ret;
    struct pollfd fds = {
           .fd = handleToFile(hPort),
           .events = POLLOUT | POLLWRNORM
    };
    ret = poll(&fds, 1, 1000);
    if (ret <= 0)
    {
        return -1;
    }
    if (!(fds.revents & (POLLOUT | POLLWRNORM)))
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
//        usleep(0);
    }
    return ret;
}


int SerialReceive(SerialHandle hPort, uint8_t *buf, int len)
{
    int ret = read(handleToFile(hPort), buf, len);
    if ((ret < 0) && (errno == EAGAIN || errno == EINTR))
    {
        return 0;
    }
    if (ret > 0)
    {
//        printf("%c\n", *buf);
    }
    return ret;
}

#endif
