/*
    Copyright 2017-2020 (C) Alexey Dynda

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
    : m_readbuf{}
    , m_writebuf{}
    , m_readbuf_size( readbuf_size )
    , m_writebuf_size( writebuf_size )
{
}


int FakeWire::read(uint8_t *data, int length, int timeout)
{
    int size = 0;
    auto startTs = std::chrono::steady_clock::now();
    do
    {
        m_readmutex.lock();
        int len_to_copy = m_readbuf.size();
        if ( len_to_copy > length - size ) len_to_copy = length - size;
        while (len_to_copy--)
        {
            data[ size ] = m_readbuf.front();
            m_readbuf.pop();
            size++;
        }
        m_readmutex.unlock();
        if ( size == length || timeout <= 0 ) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        timeout-=5;
    } while (std::chrono::steady_clock::now() - startTs <= std::chrono::milliseconds(timeout));
    return size;
}

void FakeWire::reset()
{
    std::queue<uint8_t> empty;
    std::queue<uint8_t> empty2;
    std::swap( m_readbuf, empty );
    std::swap( m_writebuf, empty2 );
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
    auto startTs = std::chrono::steady_clock::now();
    do
    {
        m_writemutex.lock();
        int len_to_copy = m_writebuf_size - m_writebuf.size();
        if ( len_to_copy > length - size ) len_to_copy = length - size;
        while (len_to_copy--)
        {
            m_writebuf.push(data[ size ]);
            size++;
        }
        m_writemutex.unlock();
        if ( size == length || timeout <= 0 ) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        timeout-=5;
    } while (std::chrono::steady_clock::now() - startTs <= std::chrono::milliseconds(timeout));
    return size;
}

void FakeWire::TransferData(int bytes)
{
    m_readmutex.lock();
    m_writemutex.lock();
    while ( bytes-- && m_writebuf.size() )
    {
        if ( m_readbuf.size() < (unsigned int)m_readbuf_size && m_enabled ) // If we don't have space or device not enabled, the data will be lost
        {
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
            m_readbuf.push( error_happened ? (m_writebuf.front() ^ 0x34 ) : m_writebuf.front() );
        }
        else
        {
            if (m_enabled)
            {
                //fprintf(stderr, "HW missed byte %d -> %d\n", m_writebuf_size, m_readbuf_size);
                m_lostBytes++;
            }
        }
        m_writebuf.pop();
    }
    m_writemutex.unlock();
    m_readmutex.unlock();
}

FakeWire::~FakeWire()
{
}
