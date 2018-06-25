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

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>

void CloseSerial(SerialHandle port)
{
    PurgeComm(port, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);
    if(port != INVALID_HANDLE_VALUE)
    {
        CloseHandle(port);
    }
}

static int SerialSettings(SerialHandle hPort, uint32_t baud)
{
    DCB PortDCB;
    PortDCB.DCBlength = sizeof (DCB);
    GetCommState (hPort, &PortDCB);

    PortDCB.BaudRate = baud;
    PortDCB.fBinary = TRUE;
    PortDCB.ByteSize = 8;
    PortDCB.Parity = NOPARITY;
    PortDCB.StopBits = ONESTOPBIT;
    if (!SetCommState (hPort, &PortDCB))
    {
        CloseSerial(hPort);
        printf("Unable to configure the serial port\n");
        return 0;
    }
    return 1;
}

static int SerialTimeouts(SerialHandle hPort)
{
    COMMTIMEOUTS CommTimeouts;
    GetCommTimeouts (hPort, &CommTimeouts);
    CommTimeouts.ReadIntervalTimeout = 100;
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    CommTimeouts.ReadTotalTimeoutConstant = 1000;
    CommTimeouts.WriteTotalTimeoutMultiplier = 0;
    CommTimeouts.WriteTotalTimeoutConstant = 1000;
    if (!SetCommTimeouts (hPort, &CommTimeouts))
    {
        CloseSerial(hPort);
        return 0;
    }
    return 1;
}

SerialHandle OpenSerial(const char* name, uint32_t baud)
{
    HANDLE hPort=CreateFile(name,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    if ( hPort != INVALID_HANDLE_VALUE )
    {
        if ( !SerialSettings(hPort, baud) ||
             !SerialTimeouts(hPort) ||
             !SetCommMask(hPort, EV_RXCHAR) )
        {
            CloseSerial(hPort);
            hPort = INVALID_HANDLE_VALUE;
            return hPort;
        }
    }
    return hPort;
}

int SerialSend(SerialHandle hPort, const uint8_t *buf, int len)
{
    DWORD tBytes;
    if (WriteFile(hPort, buf, len, &tBytes, NULL))
    {
//        printf("Write:  ");
//        printf("%c\n", buf[0]);
        return tBytes;
    }
    else
    {
        printf("error %d\n", GetLastError());
        PurgeComm(hPort, PURGE_TXABORT );
    }
    return -1;
}


int SerialReceive(SerialHandle hPort, uint8_t *buf, int len)
{
    DWORD rBytes;
    if (ReadFile(hPort,buf,len,&rBytes, NULL))
    {
//        printf("B %c\n",buf[0]);
        return rBytes;
    }
    else
    {
        printf("error %d\n", GetLastError());
        PurgeComm(hPort, PURGE_RXABORT );
    }
    return -1;
}

#endif
