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

#include "fake_wire.h"
#include <stdio.h>

FakeWire::FakeWire()
    :m_buf{0}
{
    m_writeptr = 0;
    m_readptr = 0;
}


int FakeWire::read(uint8_t *data, int length)
{
    m_mutex.lock();
    int size = 0;
    while (size < length)
    {
        if ( m_readptr == m_writeptr )
        {
            break;
        }
        data[size] = m_buf[m_readptr];
//        fprintf(stderr, "%02X ", data[size]);
        if (m_readptr >= (int)sizeof(m_buf) - 1)
            m_readptr = 0;
        else
            m_readptr++;
        size++;
    }
//    if ( size ) fprintf( stderr, "\n" );
    m_mutex.unlock();
    return size;
}


void FakeWire::reset()
{
    m_mutex.lock();
    m_writeptr = 0;
    m_readptr  = 0;
    m_mutex.unlock();
}

int FakeWire::write(const uint8_t *data, int length)
{
    int size = 0;
    m_mutex.lock();
    while (size < length)
    {
        int l_writeptr = m_writeptr + 1;
        if (l_writeptr >= (int)sizeof(m_buf))
        {
            l_writeptr = 0;
        }
        /* Atomic read */
        if (l_writeptr == m_readptr)
        {
            /* no space to write */
            break;
        }
        m_byte_counter++;
//        fprintf(stderr, "%02X ", data[size]);
        if ( m_error_byte_num != 0 && (m_byte_counter % m_error_byte_num) == 0 )
        {
            m_buf[m_writeptr] = data[size] ^ 0x34;
        }
        else
        {
            m_buf[m_writeptr] = data[size];
        }
        m_writeptr = l_writeptr;
        size++;
    }
//    if ( size ) fprintf( stderr, "\n" );
    m_mutex.unlock();
    return size;
}


FakeWire::~FakeWire()
{
}


