/*
    Copyright 2016 (C) Alexey Dynda

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/serial.h>
#include <termios.h>

#include "uart.h"

// Return 0 if successful, -1 otherwise
int uartInit(int *fd, char *devName, int baudRate)
{
   struct termios options;

//   info->bus.driver.fd = open(info->bus.driver.devName, O_RDWR | O_NOCTTY);
   // For non-blocking   What about O_NONBLOCK?
   *fd = open( devName, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
   if (*fd == -1)
   {
      printf("ERROR: Failed to open uart device %s\n", devName);
      return -1;
   }

   // Get the current options for the port
   if (tcgetattr(*fd, &options) == -1)
   {
      printf("ERROR: Failed to get uart attributes");
      return -1;
   }

   // Set the baud rates
   if (cfsetispeed(&options, baudRate) == -1)
   {
      printf("ERROR: Failed to set uart speed\n");
      return -1;
   }
   if (cfsetospeed(&options, baudRate) == -1)
   {
      printf("ERROR: Failed to set uart speed\n");
      return -1;
   }

   // Parity bits: 0
   options.c_cflag &= ~PARENB;
   options.c_cflag |= PARODD;

   // Set 8 bit
   options.c_cflag &= ~CSIZE;
   options.c_cflag |= CS8;

   // No stop bits
   options.c_cflag &= ~CSTOPB;

   // No flow control
   options.c_cflag &= ~CRTSCTS;

   // Raw input, no echo
   options.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | IEXTEN | ISIG);
   options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                        | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY | INPCK );

   // Raw output
   options.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
                       ONOCR | OFILL | OLCUC | OPOST);

   options.c_cc[VMIN] = 1;
   options.c_cc[VTIME] = 0;

   // Enable the receiver and set local mode...
   options.c_cflag |= (CLOCAL | CREAD);

   // Flush any buffered characters
   tcflush(*fd, TCIOFLUSH);

   // Set the new options for the port...
   if (tcsetattr(*fd, TCSANOW, &options) == -1)
   {
      return -1;
   }

   return 0;
}

inline
int uartRead(int *fd, uint8_t *data, int length)
{
   return read(*fd, data, length);
}


int uartWrite(int *fd, const uint8_t *data, int length)
{
   return write(*fd, data, length);
}


int uartClose(int *fd)
{
   int result;
   result = close(*fd);
   *fd = -1;
   return result;
}
