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

#ifndef _FAKE_WIRE_H_
#define _FAKE_WIRE_H_

#include <stdint.h>
#include <pthread.h>

typedef struct
{
    int writeptr;
    int readptr;
    pthread_mutex_t  lock;
    uint8_t buf[1024];
} fake_wire_t;

typedef struct
{
    fake_wire_t *rx;
    fake_wire_t *tx;
    int noise;
} fake_line_t;


int fakeWireInit(fake_line_t *fl, fake_wire_t *rx, fake_wire_t *tx, int noise);
int fakeWireRead(void *pdata, uint8_t *data, int length);
int fakeWireWrite(void *pdata, const uint8_t *data, int length);
int fakeWireClose(fake_line_t *fl);

#endif /* _FAKE_WIRE_H_ */
