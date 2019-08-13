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
#include "TinyPacket.h"
#include <stdio.h>
#include <time.h>

enum class protocol_type_t: uint8_t
{
    HDLC = 0,
    HD = 1,
};

static hdlc_crc_t s_crc = HDLC_CRC_8;
static char *s_port = nullptr;
static bool s_generatorEnabled = false;
static protocol_type_t s_protocol = protocol_type_t::HD;

static int serial_send(void *p, const void *buf, int len)
{
    return SerialSend((uintptr_t)p, buf, len);
}

static int serial_receive(void *p, void *buf, int len)
{
    return SerialReceive((uintptr_t)p, buf, len);
}

static void print_help()
{
    fprintf(stderr, "Usage: sperf -p <port> [-c <crc>]\n");
    fprintf(stderr, "Note: communication runs at 115200\n");
    fprintf(stderr, "    -p <port>\n");
    fprintf(stderr, "    --port <port>   com port to use\n");
    fprintf(stderr, "                    COM1, COM2 ...  for Windows\n");
    fprintf(stderr, "                    /dev/ttyS0, /dev/ttyS1 ...  for Linux\n");
    fprintf(stderr, "    -c <crc>\n");
    fprintf(stderr, "    --crc <crc>     crc type: 0, 8, 16, 32\n");
    fprintf(stderr, "    -g\n");
    fprintf(stderr, "    --generator     turn on packet generating\n");
}

static int parse_args(int argc, char *argv[])
{
    if ( argc < 2)
    {
        return -1;
    }
    int i = 1;
    while (i < argc)
    {
        if ( argv[i][0] != '-' )
        {
            break;
        }
        if ((!strcmp(argv[i],"-p")) || (!strcmp(argv[i],"--port")))
        {
            if (++i < argc ) s_port = argv[i]; else return -1;
        }
        else if ((!strcmp(argv[i],"-c")) || (!strcmp(argv[i],"--crc")))
        {
            if (++i >= argc ) return -1;
            switch (strtoul(argv[i], nullptr, 10) )
            {
                case 0: s_crc = HDLC_CRC_OFF; break;
                case 8: s_crc = HDLC_CRC_8; break;
                case 16: s_crc = HDLC_CRC_16; break;
                case 32: s_crc = HDLC_CRC_32; break;
                default: fprintf(stderr, "CRC type not supported\n"); return -1;
            }
        }
        else if ((!strcmp(argv[i],"-g")) || (!strcmp(argv[i],"--generator")))
        {
            s_generatorEnabled = true;
        }
        i++;
    }
    if (s_port == nullptr)
    {
        return -1;
    }
    return i;
}

//=================================== HD ============================================

static void onReceiveFrameHd(void *handle, uint16_t uid, uint8_t *pdata, int size)
{
    fprintf(stderr, "<<< Frame received UID=%04X, payload len=%d\n", uid, size);
}

static void onSendFrameHd(void *handle, uint16_t uid, uint8_t *pdata, int size)
{
    fprintf(stderr, ">>> Frame sent UID=%04X, payload len=%d\n", uid, size);
}

static int run_hd(SerialHandle port)
{
    uint8_t inBuffer[4096]{};
    STinyHdData tiny{};
    STinyHdInit init{};
    init.write_func       = serial_send;
    init.read_func        = serial_receive;
    init.pdata            = (void *)port;
    init.on_frame_cb      = onReceiveFrameHd;
    init.on_sent_cb       = onSendFrameHd;
    init.inbuf            = inBuffer;
    init.inbuf_size       = sizeof(inBuffer);
    init.timeout          = 100;
    init.crc_type         = s_crc;
    init.multithread_mode = 0;
    /* Initialize tiny hd protocol */
    tiny_hd_init( &tiny, &init  );

    /* Run main cycle forever */
    for(;;)
    {
        if (s_generatorEnabled)
        {
            Tiny::Packet<4096> packet;
            packet.put("Generated frame");
            if ( tiny_send_wait_ack(&tiny, packet.data(), packet.size()) < 0 )
            {
                fprintf( stderr, "Failed to send packet\n" );
            }
        }
        tiny_hd_run( &tiny );
    }
    tiny_hd_close(&tiny);
}

int main(int argc, char *argv[])
{
//    printf("HELLO\n");
    if ( parse_args(argc, argv) < 0 )
    {
        print_help();
        return 1;
    }

    SerialHandle hPort = OpenSerial(s_port, SERIAL_115200);

    if ( hPort == INVALID_SERIAL )
    {
        fprintf(stderr, "Error opening serial port\n");
        return 1;
    }

    int result = -1;
    switch (s_protocol)
    {
        case protocol_type_t::HD: result = run_hd(hPort); break;
        default: fprintf(stderr, "Unknown protocol type"); break;
    }
    CloseSerial(hPort);
    return result;
}
