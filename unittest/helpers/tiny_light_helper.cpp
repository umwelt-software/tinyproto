/*
    Copyright 2019 (C) Alexey Dynda

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

#include "tiny_light_helper.h"


uint32_t TinyLightHelper::s_handleOffset;

TinyLightHelper::TinyLightHelper(FakeChannel * channel,
                               int rx_buf_size)
    : m_handle{}
{
    s_handleOffset = (uint8_t *)this - (uint8_t *)(&m_handle);
    m_channel = channel;

    tiny_light_init(&m_handle, write_data, read_data, this);
}

int TinyLightHelper::send(uint8_t *buf, int len)
{
    return tiny_light_send( &m_handle, buf, len );
}

int TinyLightHelper::read(uint8_t *buf, int len)
{
    return tiny_light_read( &m_handle, buf, len );
}

int TinyLightHelper::read_data(void * appdata, void * data, int length)
{
    TinyLightHelper  *helper = reinterpret_cast<TinyLightHelper *>(appdata);
    return helper->m_channel->read((uint8_t *)data, length);
}


int TinyLightHelper::write_data(void * appdata, const void * data, int length)
{
    TinyLightHelper *helper = reinterpret_cast<TinyLightHelper *>(appdata);
    return helper->m_channel->write((const uint8_t *)data, length);
}

TinyLightHelper::~TinyLightHelper()
{
    tiny_light_close( &m_handle );
}

