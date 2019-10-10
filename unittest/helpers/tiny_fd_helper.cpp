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

#include "tiny_fd_helper.h"

TinyHelperFd::TinyHelperFd(FakeChannel * channel,
                           int rxBufferSize,
                           const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb,
                           bool  multithread_mode,
                           int timeout)
    :IBaseHelper(channel, rxBufferSize)
    ,m_onRxFrameCb(onRxFrameCb)
{
    STinyFdInit   init = {0};
    init.write_func       = write_data;
    init.read_func        = read_data;
    init.pdata            = this;
    init.on_frame_cb      = onRxFrame;
    init.on_sent_cb       = onTxFrame;
    init.buffer           = m_buffer;
    init.buffer_size      = rxBufferSize;
    init.window_frames    = 8;
    init.timeout          = timeout ? : 2000;
    init.retries          = 2;
    init.crc_type         = HDLC_CRC_16;
    init.multithread_mode = multithread_mode;

    tiny_fd_init( &m_handle, &init  );
}

int TinyHelperFd::send(uint8_t *buf, int len)
{
    return tiny_fd_send(m_handle, buf, len);
}

int TinyHelperFd::run_tx()
{
    return tiny_fd_run_tx(m_handle, 10);
}

int TinyHelperFd::run_rx()
{
    return tiny_fd_run_rx(m_handle, 10);
}

void  TinyHelperFd::onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len)
{
    TinyHelperFd * helper = reinterpret_cast<TinyHelperFd *>(handle);
    helper->m_rx_count++;
    if (helper->m_onRxFrameCb)
    {
        helper->m_onRxFrameCb(uid, buf, len);
    }
}

void  TinyHelperFd::onTxFrame(void *handle, uint16_t uid, uint8_t * buf, int len)
{
    TinyHelperFd * helper = reinterpret_cast<TinyHelperFd *>(handle);
    helper->m_tx_count++;
}

TinyHelperFd::~TinyHelperFd()
{
    stop();
    tiny_fd_close( m_handle );
}

