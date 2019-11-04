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
#include <thread>

FakeWire::FakeWire(int readbuf_size, int writebuf_size)
    : m_readbuf( new uint8_t[readbuf_size] )
    , m_writebuf( new uint8_t[writebuf_size] )
    , m_readbuf_size( readbuf_size )
    , m_writebuf_size( writebuf_size )
{
}


int FakeWire::read(uint8_t *data, int length, int timeout)
{
    int size = 0;
    m_readmutex.lock();
    auto startTs = std::chrono::steady_clock::now();
    do
    {
        int len_to_copy = m_readlen;
        if ( len_to_copy > length - size ) len_to_copy = length - size;
        while (len_to_copy--)
        {
            int read_index = m_readptr;
            data[ size ] = m_readbuf[ read_index ];
            m_readptr++; if ( m_readptr >= m_readbuf_size ) m_readptr -= m_readbuf_size;
            m_readlen--;
            size++;
        }
        if ( size == length || timeout == 0 ) break;
        m_readmutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        timeout--;
        m_readmutex.lock();
    } while (std::chrono::steady_clock::now() - startTs <= std::chrono::milliseconds(timeout));
    m_readmutex.unlock();
    return size;
}

void FakeWire::reset()
{
    m_writeptr = 0;
    m_readptr  = 0;
    m_writelen = 0;
    m_readlen = 0;
}

void FakeWire::flush()
{
    m_readmutex.lock();
    m_writemutex.lock();
    reset();
    m_writemutex.unlock();
    m_readmutex.unlock();
}

int FakeWire::write(const uint8_t *data, int length, int timeout)
{
    int size = 0;
    m_writemutex.lock();
    auto startTs = std::chrono::steady_clock::now();
    do
    {
        int len_to_copy = m_writebuf_size - m_writelen;
        if ( len_to_copy > length - size ) len_to_copy = length - size;
        while (len_to_copy--)
        {
            int write_index = m_writeptr + m_writelen;
            if ( write_index >= m_writebuf_size ) write_index -= m_writebuf_size;
            m_writebuf[ write_index ] = data[ size ];
            m_writelen++;
            size++;
        }
        if ( size == length || !timeout ) break;
        m_writemutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        timeout--;
        m_writemutex.lock();
    } while (std::chrono::steady_clock::now() - startTs <= std::chrono::milliseconds(timeout));
    m_writemutex.unlock();
    return size;
}

void FakeWire::TransferData(int bytes)
{
    m_readmutex.lock();
    m_writemutex.lock();
    while ( bytes-- && m_writelen )
    {
        if ( m_readlen < m_readbuf_size && m_enabled ) // If we don't have space or device not enabled, the data will be lost
        {
            int read_index = m_readptr + m_readlen;
            if ( read_index >= m_readbuf_size ) read_index -= m_readbuf_size;
            int write_index = m_writeptr;
            m_byte_counter++;
            bool error_happened = false;
            for (auto& err: m_errors)
            {
                if ( m_byte_counter >= err.first &&
                     err.count != 0 &&
                    (m_byte_counter - err.first) % err.period == 0 )
                {
                    err.count--;
                    error_happened = true;
                    break;
                }
            }
            m_readbuf[ read_index ] = error_happened ? ( m_writebuf[write_index] ^ 0x34 ) : m_writebuf[write_index];
            m_readlen++;
        }
        else
        {
            if (m_enabled)
            {
                //fprintf(stderr, "HW missed byte %d -> %d\n", m_writebuf_size, m_readbuf_size);
                m_lostBytes++;
            }
        }
        m_writelen--;
        m_writeptr++;
        if ( m_writeptr >= m_writebuf_size ) m_writeptr -= m_writebuf_size;
    }
    m_writemutex.unlock();
    m_readmutex.unlock();
}

FakeWire::~FakeWire()
{
    delete[] m_writebuf;
    delete[] m_readbuf;
}
