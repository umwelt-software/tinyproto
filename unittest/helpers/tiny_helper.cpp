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

#include "tiny_helper.h"


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

