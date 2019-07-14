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
                               const std::function<void(uint8_t*,int)> &onRxFrameCb,
                               int rx_buf_size)
    : m_handle{}
    , m_onRxFrameCb(onRxFrameCb)
{
    s_handleOffset = (uint8_t *)this - (uint8_t *)(&m_handle);
    m_channel = channel;

    m_handle.user_data = this;
    m_handle.send_tx = write_data;
    m_handle.on_frame_data = onRxFrame;
    m_handle.rx_buf = malloc( rx_buf_size );
    m_handle.rx_buf_size = rx_buf_size;
    hdlc_init( &m_handle );
}

int TinyHdlcHelper::send(uint8_t *buf, int len)
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


int TinyHdlcHelper::process_rx_bytes()
{
    uint8_t byte;
    while ( m_channel->read(&byte, 1) == 1 )
    {
        int res;
        do
        {
            res = hdlc_on_rx_data( &m_handle, &byte, 1 );
        } while (res == 0);
        if (res < 0)
        {
            return res;
        }
    }
    return 0;
}

int TinyHdlcHelper::onRxFrame(void *handle, void * buf, int len)
{
    TinyHdlcHelper * helper = reinterpret_cast<TinyHdlcHelper *>( ((uint8_t *)handle) - s_handleOffset );
    if (helper->m_onRxFrameCb)
    {
        helper->m_onRxFrameCb((uint8_t *)buf, len);
    }
    return 0;
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
    hdlc_close( &m_handle );
    free( m_handle.rx_buf );
}

