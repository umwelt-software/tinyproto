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

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include "uart.h"
#include "tiny_layer2.h"


int read_serial(void* fd, uint8_t *data, int size)
{
    return uartRead((int *)fd, data, size);
}


int write_serial(void* fd, const uint8_t *data, int size)
{
    return uartWrite((int *)fd, data, size);
}


static STinyData s_tiny;


int main(int argc, char *argv[])
{
    int fd;
    if (argc < 2)
    {
        printf("Usage %s /dev/ttyS1\n", argv[0]);
        exit(1);
    }
    if (uartInit(&fd, argv[1], B9600) < 0)
    {
        printf("Error opening uart %s\n", argv[1]);
        exit(1);
    };
    tiny_init(&s_tiny, &write_serial, &read_serial, &fd);
    tiny_set_fcs_bits(&s_tiny, 0);
    for(int i=0; i<10; i++)
    {
        uint8_t buf[16];
        strcpy((char*)buf, "Welcome\n");
        int len;
        len = tiny_send(&s_tiny, 0, buf, (int)strlen((char*)buf), TINY_FLAG_WAIT_FOREVER);
        if (len > 0)
        {
            buf[len] = '\0';
            printf("%s", buf);
        }
        do
        {
            len = tiny_read(&s_tiny, 0, buf, sizeof(buf), TINY_FLAG_WAIT_FOREVER);
            if (len > 0)
            {
                buf[len] = '\0';
                printf("%s", buf);
            }
        }
        while (len == TINY_ERR_OUT_OF_SYNC);
    }
    tiny_close(&s_tiny);
    uartClose(&fd);
    printf("[DONE]\n");
    return 0;
}
