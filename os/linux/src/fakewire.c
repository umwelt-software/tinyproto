/*
    Copyright 2016-2017 (C) Alexey Dynda

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

#include <stdlib.h>
#include <stdio.h>


#include "fakewire.h"

int fakeWireInit(fake_line_t *fl, fake_wire_t *rx, fake_wire_t *tx, int noise)
{
    if (!rx || !tx || !fl)
        return -1;
    fl->tx = tx;
    fl->rx = rx;
    fl->noise = noise;
    tx->writeptr = 0;
    tx->readptr = 0;
    rx->writeptr = 0;
    rx->readptr = 0;
    pthread_mutex_init(&tx->lock, NULL);
    return 0;
}

int fakeWireRead(void *pdata, uint8_t *data, int length)
{
    fake_line_t *fl = (fake_line_t *)pdata;
    pthread_mutex_lock( &fl->rx->lock );
    int size = 0;
    while (size < length)
    {
        if ( fl->rx->readptr == fl->rx->writeptr )
        {
            break;
        }
        data[size] = fl->rx->buf[fl->rx->readptr];
        fprintf(stderr, "%p: IN %02X\n", pdata, data[size]);
/*        if ((fl->noise) && ((rand() & 0xFF) == 0))
            data[size]^= 0x28;*/
        /* Atomic move of the pointer */
        if (fl->rx->readptr >= sizeof(fl->rx->buf) - 1)
            fl->rx->readptr = 0;
        else
            fl->rx->readptr++;
        size++;
    }
    pthread_mutex_unlock( &fl->rx->lock );
    return size;
}


int fakeWireWrite(void *pdata, const uint8_t *data, int length)
{
    fake_line_t *fl = (fake_line_t *)pdata;
    int size = 0;
    pthread_mutex_lock( &fl->tx->lock );
    while (size < length)
    {
        int writeptr = fl->tx->writeptr + 1;
        if (writeptr >= sizeof(fl->tx->buf))
        {
            writeptr = 0;
        }
        /* Atomic read */
        if (writeptr == fl->tx->readptr)
        {
            /* no space to write */
            break;
        }
        fprintf(stderr, "%p: OUT %02X\n", pdata, data[size]);
        fl->tx->buf[fl->tx->writeptr] = data[size];
        fl->tx->writeptr = writeptr;
        size++;
    }
    pthread_mutex_unlock( &fl->tx->lock );
    return size;
}


int fakeWireClose(fake_line_t *fl)
{
   pthread_mutex_destroy(&fl->tx->lock);
   return 0;
}
