/*
    Copyright 2019 (C) Alexey Dynda

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
#include "proto/half_duplex/tiny_hd.h"
#include <stdio.h>
#include <time.h>

static void onReceiveFrame(void *handle, uint16_t uid, uint8_t *pdata, int size)
{
    printf("Frame received UID=%04X, payload len=%d\n", uid, size);
}


static int serial_send(void *p, const void *buf, int len)
{
    return SerialSend((uintptr_t)p, buf, len);
}

static int serial_receive(void *p, void *buf, int len)
{
    return SerialReceive((uintptr_t)p, buf, len);
}

int main(int argc, char *argv[])
{
    if ( argc < 2)
    {
        printf("Usage: sperf Serial [crc]\n");
        printf("Note: communication runs at 115200\n");
        printf("    Serial  - com port to use\n");
        printf("        COM1, COM2 ...  for Windows\n");
        printf("        /dev/ttyS0, /dev/ttyS1 ...  for Linux\n");
        printf("    crc     - crc type: 0, 8, 16, 32\n");
        return 1;
    }

    SerialHandle hPort = OpenSerial(argv[1], SERIAL_115200);

    if ( hPort == INVALID_SERIAL )
    {
        printf("Error opening serial port\n");
        return 1;
    }

//    uint8_t outBuffer[4096]{};
    uint8_t inBuffer[4096]{};
    STinyHdData tiny{};
    STinyHdInit init{};
    init.write_func       = serial_send;
    init.read_func        = serial_receive;
    init.pdata            = (void *)hPort;
    init.on_frame_cb      = onReceiveFrame;
    init.inbuf            = inBuffer;
    init.inbuf_size       = sizeof(inBuffer);
    init.timeout          = 100;
    init.crc_type         = HDLC_CRC_8;
    if ( argc == 3 )
    {
        switch (strtoul(argv[2], nullptr, 10) )
        {
            case 0: init.crc_type = HDLC_CRC_OFF; break;
            case 8: init.crc_type = HDLC_CRC_8; break;
            case 16: init.crc_type = HDLC_CRC_16; break;
            case 32: init.crc_type = HDLC_CRC_32; break;
            default: printf("CRC type not supported\n"); return -1;
        }
    }
    init.multithread_mode = 0;

    /* Initialize tiny hd protocol */
    tiny_hd_init( &tiny, &init  );

    /* Run main cycle forever */
    for(;;)
    {
        tiny_hd_run( &tiny );
    }
    tiny_hd_close(&tiny);
    CloseSerial(hPort);
    return 0;
}
