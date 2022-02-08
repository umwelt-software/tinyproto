/*
    Copyright 2017-2020,2022 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

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

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/

#include "fake_wire.h"
#include <cstdio>
#include <thread>
#include <chrono>


//////////////////////////////////////////////////////////////////////////////////////

enum
{
    HAS_SPACE = 0x01,
    HAS_DATA = 0x02,
};

TxHardwareBlock::TxHardwareBlock(int size)
    : m_size( size )
{
    tiny_events_create( &m_events );
    tiny_mutex_create( &m_mutex );
    tiny_events_set( &m_events, HAS_SPACE );
}

TxHardwareBlock::~TxHardwareBlock()
{
    tiny_mutex_destroy( &m_mutex );
    tiny_events_destroy( &m_events );
}

int TxHardwareBlock::Write(const void *buffer, int size, uint32_t timeout)
{
    int written = 0;
    while ( size > 0 )
    {
        uint32_t ts = tiny_millis();
        if ( tiny_events_wait(&m_events, HAS_SPACE, EVENT_BITS_LEAVE, timeout) == 0 )
        {
            break;
        }
        uint32_t delta = (uint32_t)(tiny_millis() - ts);
        timeout = timeout < delta ? 0: (timeout - delta);
        tiny_mutex_lock( &m_mutex );
        while (size > 0 && (int)m_queue.size() < m_size)
        {
            m_queue.push( *(const uint8_t *)buffer );
//            fprintf(stderr, "%02X\n", *(const uint8_t *)buffer);
            buffer = (const void *)((const uint8_t *)buffer + 1);
            size--;
            written++;
        }
        if ( (int)m_queue.size() > 0 )
        {
            tiny_events_set( &m_events, HAS_DATA );
        }
        if ( (int)m_queue.size() == m_size )
        {
            tiny_events_clear( &m_events, HAS_SPACE );
        }
        tiny_mutex_unlock( &m_mutex );
    }
    return written;
}

int TxHardwareBlock::Peek(uint8_t &byte)
{
    if ( !m_enabled )
    {
        return 0;
    }
    if ( tiny_events_wait(&m_events, HAS_DATA, EVENT_BITS_LEAVE, 0) == 0 )
    {
        return 0;
    }
    tiny_mutex_lock( &m_mutex );
    byte = m_queue.front();
    m_queue.pop();
    if ( (int)m_queue.size() < m_size )
    {
        tiny_events_set( &m_events, HAS_SPACE );
    }
    if ( (int)m_queue.size() == 0 )
    {
        tiny_events_clear( &m_events, HAS_DATA );
    }
    tiny_mutex_unlock( &m_mutex );
//    fprintf(stderr, "TX: %02X\n", byte);
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////

RxHardwareBlock::RxHardwareBlock(int size)
    : m_size( size )
{
    tiny_events_create( &m_events );
    tiny_mutex_create( &m_mutex );
    tiny_events_set( &m_events, HAS_SPACE );
}

RxHardwareBlock::~RxHardwareBlock()
{
    tiny_mutex_destroy( &m_mutex );
    tiny_events_destroy( &m_events );
}

int RxHardwareBlock::Read(void *buffer, int size, uint32_t timeout)
{
    int read = 0;
    while ( size > 0 )
    {
        uint32_t ts = tiny_millis();
        if ( tiny_events_wait(&m_events, HAS_DATA, EVENT_BITS_LEAVE, timeout) == 0 )
        {
            break;
        }
        uint32_t delta = (uint32_t)(tiny_millis() - ts);
        timeout = timeout < delta ? 0: (timeout - delta);
        tiny_mutex_lock( &m_mutex );
        while (size > 0 && (int)m_queue.size() > 0)
        {
            *(uint8_t *)buffer = m_queue.front();
            m_queue.pop();
            buffer = (void *)((uint8_t *)buffer + 1);
            size--;
            read++;
        }
        if ( (int)m_queue.size() == 0 )
        {
            tiny_events_clear( &m_events, HAS_DATA );
        }
        if ( (int)m_queue.size() < m_size )
        {
            tiny_events_set( &m_events, HAS_SPACE );
        }
        tiny_mutex_unlock( &m_mutex );
    }
    return read;
}

int RxHardwareBlock::Put(uint8_t byte)
{
    if ( !m_enabled)
    {
        return 0;
    }
    if ( tiny_events_wait(&m_events, HAS_SPACE, EVENT_BITS_LEAVE, 0) == 0 )
    {
        return 0;
    }
    tiny_mutex_lock( &m_mutex );
    m_queue.push( byte );
    if ( (int)m_queue.size() == m_size )
    {
        tiny_events_clear( &m_events, HAS_SPACE );
    }
    if ( (int)m_queue.size() > 0 )
    {
        tiny_events_set( &m_events, HAS_DATA );
    }
    tiny_mutex_unlock( &m_mutex );
    return 1;
}

bool RxHardwareBlock::WaitUntilRxCount(int count, uint32_t timeout)
{
    for ( ;; )
    {
        tiny_mutex_lock( &m_mutex );
        int size = (int)m_queue.size();
        tiny_mutex_unlock( &m_mutex );
        if ( size >= count )
        {
            return true;
        }
        tiny_sleep( 1 );
        if ( !timeout )
            break;
        timeout--;
    }
    return false;
}

void RxHardwareBlock::Flush()
{
    tiny_mutex_lock( &m_mutex );
    tiny_events_clear( &m_events, HAS_DATA );
    tiny_events_set( &m_events, HAS_SPACE );
    while ( m_queue.size() > 0 )
    {
        m_queue.pop();
    }
    tiny_mutex_unlock( &m_mutex );
}


//////////////////////////////////////////////////////////////////////////////////////

FakeWire::FakeWire()
{
}

FakeWire::~FakeWire()
{
    for (auto tx: m_tx)
    {
        delete tx;
    }
    for (auto rx: m_rx)
    {
        delete rx;
    }
}

RxHardwareBlock *FakeWire::CreateRxHardware(int size)
{
    RxHardwareBlock * block = new RxHardwareBlock(size);
    m_rx.push_back(block);
    return block;
}

TxHardwareBlock *FakeWire::CreateTxHardware(int size)
{
    TxHardwareBlock * block = new TxHardwareBlock(size);
    m_tx.push_back(block);
    return block;
}

void FakeWire::TransferData(int bytes)
{
    while ( bytes-- )
    {
        bool has_data = false;
        uint8_t data = 0;
        for (auto tx: m_tx)
        {
            uint8_t d;
            if (tx->Peek(d))
            {
                data = data | d;
                has_data = true;
            }
        }
        if ( !has_data )
        {
            break;
        }
//        fprintf(stderr, "*T: %02X\n", data);
        if ( data == 0x7E ) { if ( ++cnt >=3 ) *((uint8_t *)0) = 1; } else cnt = 0;
        m_byte_counter++;
        bool error_happened = false;
        for ( auto &err : m_errors )
        {
            if ( m_byte_counter >= err.first && err.count != 0 && (m_byte_counter - err.first) % err.period == 0 )
            {
                err.count--;
                error_happened = true;
                break;
            }
        }
        data = error_happened ? (data ^ 0x34) : data;
        for (auto rx: m_rx)
        {
            if (!rx->Put(data))
            {
                m_lostBytes++;
            }
        }
    }
}

