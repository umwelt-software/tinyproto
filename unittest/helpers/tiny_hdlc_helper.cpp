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

#include "tiny_hdlc_helper.h"


uint32_t TinyHdlcHelper::s_handleOffset;

TinyHdlcHelper::TinyHdlcHelper(FakeChannel * channel,
                               const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb)
    :m_onRxFrameCb(onRxFrameCb)
{
    s_handleOffset = (uint8_t *)this - (uint8_t *)(&m_handle);
    m_channel = channel;
    hdlc_init( &m_handle, TinyHdlcHelper::write_data, this );
    if (m_onRxFrameCb)
    {
//        tiny_set_callbacks( &m_handle, onRxFrame, nullptr );
    }
//    tiny_set_fcs_bits( &m_handle, 16);
}

int TinyHdlcHelper::send(uint16_t *uid, uint8_t *buf, int len, uint8_t flags)
{
    int result = hdlc_send( &m_handle, (uint8_t *)buf, len);
    if ( result <= 0 )
    {
        return result;
    }
    int temp_result;
    result = 0;
    do
    {
        temp_result = hdlc_run_tx( &m_handle );
        result += temp_result;
    } while ( temp_result > 0 );
    return result;
}


int TinyHdlcHelper::read(uint16_t *uid, uint8_t *buf, int maxLen, uint8_t flags)
{
    return 0;
//    return tiny_read( &m_handle, uid, buf, maxLen, flags);
}

int TinyHdlcHelper::on_rx_byte(uint8_t *buf, int len, uint8_t byte)
{
    return hdlc_on_rx_data( &m_handle, &byte, 1 );
}

void  TinyHdlcHelper::onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len)
{
    TinyHdlcHelper * helper = reinterpret_cast<TinyHdlcHelper *>( ((uint8_t *)handle) - s_handleOffset );
    if (helper->m_onRxFrameCb)
    {
        helper->m_onRxFrameCb(uid, buf, len);
    }
}

int TinyHdlcHelper::read_data(void * appdata, uint8_t * data, int length)
{
    TinyHdlcHelper  *helper = reinterpret_cast<TinyHdlcHelper *>(appdata);
    return helper->m_channel->read(data, length);
}


int TinyHdlcHelper::write_data(void * appdata, const void * data, int length)
{
    TinyHdlcHelper *helper = reinterpret_cast<TinyHdlcHelper *>(appdata);
    return helper->m_channel->write((const uint8_t *)data, length);
}

TinyHdlcHelper::~TinyHdlcHelper()
{
    // TODO tiny_close( &m_handle );
}

