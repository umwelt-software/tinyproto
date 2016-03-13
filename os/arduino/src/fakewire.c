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

#include <stdlib.h>


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
    return 0;
}

int fakeWireRead(void *pdata, uint8_t *data, int length)
{
    fake_line_t *fl = (fake_line_t *)pdata;
    int size = 0;
    while ((fl->rx->writeptr != fl->rx->readptr) && (size < length))
    {
        data[size] = fl->rx->buf[fl->rx->readptr];
/*        if ((fl->noise) && ((rand() & 0xFF) == 0))
            data[size]^= 0x28;*/
        /* Atomic move of the pointer */
        if (fl->rx->readptr >= sizeof(fl->rx->buf))
            fl->rx->readptr = 0;
        else
            fl->rx->readptr++;
        size++;
    }
    return size;
}


int fakeWireWrite(void *pdata, const uint8_t *data, int length)
{
    fake_line_t *fl = (fake_line_t *)pdata;
    int size = 0;
    int lastpos;
    while (size < length)
    {
        /* Atomic read */
        lastpos = fl->tx->readptr;
        lastpos = (lastpos == 0 ? sizeof(fl->tx->buf) : lastpos) - 1;
        if (fl->tx->writeptr == lastpos)
            break;
        fl->tx->buf[fl->tx->writeptr] = data[size];
        /* Making atomic change */
        if (fl->tx->writeptr >= sizeof(fl->tx->buf))
            fl->tx->writeptr = 0;
        else
            fl->tx->writeptr++;
        size++;
    }
    return size;
}


int fakeWireClose(fake_line_t *fl)
{
   return 0;
}
