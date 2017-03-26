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

#include "fake_wire.h"
#include <stdlib.h>

#include <stdio.h>


FakeWire::FakeWire()
{
    m_writeptr = 0;
    m_readptr = 0;
    pthread_mutex_init(&m_lock, NULL);
}


int FakeWire::read(uint8_t *data, int length)
{
    pthread_mutex_lock( &m_lock );
    int size = 0;
    while (size < length)
    {
        if ( m_readptr == m_writeptr )
        {
            break;
        }
        data[size] = m_buf[m_readptr];
//        fprintf(stderr, "%p: IN %02X\n", pdata, data[size]);
/*        if ((fl->noise) && ((rand() & 0xFF) == 0))
            data[size]^= 0x28;*/
        /* Atomic move of the pointer */
        if (m_readptr >= (int)sizeof(m_buf) - 1)
            m_readptr = 0;
        else
            m_readptr++;
        size++;
    }
    pthread_mutex_unlock( &m_lock );
    return size;
}


void FakeWire::reset()
{
    pthread_mutex_lock( &m_lock );
    m_writeptr = 0;
    m_readptr  = 0;
    pthread_mutex_unlock( &m_lock );
}

int FakeWire::write(const uint8_t *data, int length)
{
    int size = 0;
    pthread_mutex_lock( &m_lock );
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
//        fprintf(stderr, "%p: OUT %02X\n", pdata, data[size]);
        m_buf[m_writeptr] = data[size];
        m_writeptr = l_writeptr;
        size++;
    }
    pthread_mutex_unlock( &m_lock );
    return size;
}


FakeWire::~FakeWire()
{
   pthread_mutex_destroy(&m_lock);
}


FakeChannel::FakeChannel(FakeWire *tx, FakeWire *rx)
{
    m_rx = rx;
    m_tx = tx;
}

FakeChannel::~FakeChannel()
{
}


int FakeChannel::read(void * appdata, uint8_t * data, int length)
{
    FakeChannel  *channel = reinterpret_cast<FakeChannel *>(appdata);
    return channel->m_rx->read(data, length);
}


int FakeChannel::write(void * appdata, const uint8_t * data, int length)
{
    FakeChannel  *channel = reinterpret_cast<FakeChannel *>(appdata);
    return channel->m_tx->write(data, length);
}


int FakeChannel::read(uint8_t * data, int length)
{
    return m_rx->read(data, length);
}


int FakeChannel::write(const uint8_t * data, int length)
{
    return m_tx->write(data, length);
}



uint32_t TinyHelper::s_handleOffset;

TinyHelper::TinyHelper(FakeChannel * channel,
                       const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb)
{
    s_handleOffset = (uint8_t *)this - (uint8_t *)(&m_handle);
    m_channel = channel;
    m_onRxFrameCb = onRxFrameCb;
    tiny_init( &m_handle, TinyHelper::write_data, TinyHelper::read_data, this );
    if (m_onRxFrameCb)
    {
        tiny_set_callbacks( &m_handle, onRxFrame, nullptr );
    }
    tiny_set_fcs_bits( &m_handle, 16);
}

void TinyHelper::enableUid()
{
    tiny_enable_uid( &m_handle, 1 );
}

int TinyHelper::send(uint16_t *uid, uint8_t *buf, int len, uint8_t flags)
{
    return tiny_send( &m_handle, uid, (uint8_t *)buf, len, flags);
}


int TinyHelper::read(uint16_t *uid, uint8_t *buf, int maxLen, uint8_t flags)
{
    return tiny_read( &m_handle, uid, buf, maxLen, flags);
}

int TinyHelper::on_rx_byte(uint8_t *buf, int len, uint8_t byte)
{
    return tiny_on_rx_byte( &m_handle, buf, len, byte );
}

void  TinyHelper::onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len)
{
    TinyHelper * helper = reinterpret_cast<TinyHelper *>( ((uint8_t *)handle) - s_handleOffset );
    if (helper->m_onRxFrameCb)
    {
        helper->m_onRxFrameCb(uid, buf, len);
    }
}


int TinyHelper::read_data(void * appdata, uint8_t * data, int length)
{
    TinyHelper  *helper = reinterpret_cast<TinyHelper *>(appdata);
    return helper->m_channel->read(data, length);
}


int TinyHelper::write_data(void * appdata, const uint8_t * data, int length)
{
    TinyHelper *helper = reinterpret_cast<TinyHelper *>(appdata);
    return helper->m_channel->write(data, length);
}

TinyHelper::~TinyHelper()
{
    tiny_close( &m_handle );
}

