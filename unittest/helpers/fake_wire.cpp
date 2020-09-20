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

enum
{
    EV_WR_ROOM_AVAIL = 0x01,
    EV_RD_DATA_AVAIL = 0x02,
    EV_TR_READY      = 0x04,
};


FakeWire::FakeWire(int readbuf_size, int writebuf_size)
    : m_readbuf{}
    , m_writebuf{}
    , m_readbuf_size( readbuf_size )
    , m_writebuf_size( writebuf_size )
{
    tiny_events_create( &m_events );
    tiny_events_clear( &m_events, EV_RD_DATA_AVAIL );
    tiny_events_set( &m_events, EV_WR_ROOM_AVAIL );
}


int FakeWire::read(uint8_t *data, int length, int timeout)
{
    int size = 0;
    if ( tiny_events_wait( &m_events, EV_RD_DATA_AVAIL, EVENT_BITS_CLEAR, timeout ) == 0 )
    {
        return 0;
    }
    m_readmutex.lock();
    while ( m_readbuf.size() && size < length )
    {
        data[ size ] = m_readbuf.front();
        m_readbuf.pop();
        size++;
    }
    if ( m_readbuf.size() )
    {
        tiny_events_set( &m_events, EV_RD_DATA_AVAIL );
    }
    m_readmutex.unlock();
    return size;
}

int FakeWire::write(const uint8_t *data, int length, int timeout)
{
    int size = 0;
    if ( tiny_events_wait( &m_events, EV_WR_ROOM_AVAIL, EVENT_BITS_CLEAR, timeout ) == 0 )
    {
        return 0;
    }
    m_writemutex.lock();
    while ( m_writebuf.size() < (unsigned int)m_writebuf_size && size < length )
    {
        m_writebuf.push( data[ size ] );
        size++;
    }
    if ( m_writebuf.size() < (unsigned int)m_writebuf_size )
    {
        tiny_events_set( &m_events, EV_WR_ROOM_AVAIL );
    }
    if ( size )
    {
        tiny_events_set( &m_events, EV_TR_READY );
    }
    m_writemutex.unlock();
    return size;
}

void FakeWire::TransferData(int bytes)
{
    bool room_avail = false;
    bool data_avail = false;
    if ( tiny_events_wait( &m_events, EV_TR_READY, EVENT_BITS_CLEAR, 0 ) == 0 )
    {
        return;
    }
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
            data_avail = true;
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
        room_avail = true;
    }
    if ( m_writebuf.size() )
    {
        tiny_events_set( &m_events, EV_TR_READY );
    }
    m_writemutex.unlock();
    m_readmutex.unlock();
    if ( room_avail )
    {
        tiny_events_set( &m_events, EV_WR_ROOM_AVAIL );
    }
    if ( data_avail )
    {
        tiny_events_set( &m_events, EV_RD_DATA_AVAIL );
    }
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

FakeWire::~FakeWire()
{
    tiny_events_destroy( &m_events );
}
