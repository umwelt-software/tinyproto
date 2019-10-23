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

#include "tiny_hd_helper.h"

TinyHelperHd::TinyHelperHd(FakeChannel * channel,
                           int rxBufferSize,
                           const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb,
                           bool  multithread_mode)
    :IBaseHelper(channel, rxBufferSize)
    ,m_onRxFrameCb(onRxFrameCb)
{
    STinyHdInit   init = {0};
    init.write_func       = write_data;
    init.read_func        = read_data;
    init.pdata            = this;
    init.on_frame_cb      = onRxFrame;
    init.inbuf            = m_buffer;
    init.inbuf_size       = rxBufferSize;
    init.timeout          = 2000;
    init.crc_type         = HDLC_CRC_16;
    init.multithread_mode = multithread_mode;

    tiny_hd_init( &m_handle, &init  );
}

int TinyHelperHd::send_wait_ack(uint8_t *buf, int len)
{
    return tiny_send_wait_ack(&m_handle, buf, len);
}

int TinyHelperHd::run_rx()
{
    return tiny_hd_run(&m_handle);
}

int TinyHelperHd::run_tx()
{
    if ( m_handle.multithread_mode )
    {
        return tiny_hd_run_tx(&m_handle);
    }
    usleep( 1000 );
    return TINY_SUCCESS;
}

void TinyHelperHd::wait_until_rx_count(int count, uint32_t timeout)
{
    while ( m_rx_count != count && timeout-- ) usleep(1000);
}

void  TinyHelperHd::onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len)
{
    TinyHelperHd * helper = reinterpret_cast<TinyHelperHd *>(handle);
    helper->m_rx_count++;
    if (helper->m_onRxFrameCb)
    {
        helper->m_onRxFrameCb(uid, buf, len);
    }
}

TinyHelperHd::~TinyHelperHd()
{
    stop();
    tiny_hd_close( &m_handle );
}

