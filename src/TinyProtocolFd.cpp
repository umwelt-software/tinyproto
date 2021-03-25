/*
    Copyright 2019-2020 (C) Alexey Dynda

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

#if defined(ARDUINO)
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#endif

#include "TinyProtocolFd.h"

namespace tinyproto
{
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void IFd::onReceiveInternal(void *handle, uint8_t *pdata, int size)
{
    (reinterpret_cast<IFd *>(handle))->onReceive(pdata, size);
}

void IFd::onSendInternal(void *handle, uint8_t *pdata, int size)
{
    (reinterpret_cast<IFd *>(handle))->onSend(pdata, size);
}

void IFd::begin()
{
    tiny_fd_init_t init{};
    init.pdata = this;
    init.on_frame_cb = onReceiveInternal;
    init.on_sent_cb = onSendInternal;
    init.buffer = m_buffer;
    init.buffer_size = m_bufferSize;
    init.window_frames = m_window;
    init.send_timeout = m_sendTimeout;
    init.retry_timeout = 200;
    init.retries = 2;
    init.crc_type = m_crc;

    tiny_fd_init(&m_handle, &init);
}

void IFd::end()
{
    if ( m_bufferSize == 0 )
        return;
    tiny_fd_close(m_handle);
}

int IFd::write(const char *buf, int size)
{
    return tiny_fd_send_packet(m_handle, buf, size);
}

int IFd::write(const IPacket &pkt)
{
    return tiny_fd_send_packet(m_handle, pkt.m_buf, pkt.m_len);
}

int IFd::run_rx(const void *data, int len)
{
    return tiny_fd_on_rx_data(m_handle, data, len);
}

int IFd::run_rx(read_block_cb_t read_func)
{
    uint8_t buf[4];
    int len = read_func(m_userData, buf, sizeof(buf));
    if ( len <= 0 )
    {
        return len;
    }
    return tiny_fd_on_rx_data(m_handle, buf, len);
}

int IFd::run_tx(void *data, int max_size)
{
    return tiny_fd_get_tx_data(m_handle, data, max_size);
}

int IFd::run_tx(write_block_cb_t write_func)
{
    uint8_t buf[4];
    int len = tiny_fd_get_tx_data(m_handle, buf, sizeof(buf));
    if ( len <= 0 )
    {
        return len;
    }
    uint8_t *ptr = buf;
    while ( len )
    {
        int result = write_func(m_userData, ptr, len);
        if ( result < 0 )
        {
            return result;
        }
        len -= result;
        ptr += result;
    }
    return TINY_SUCCESS;
}

void IFd::disableCrc()
{
    m_crc = HDLC_CRC_OFF;
}

void IFd::enableCrc(hdlc_crc_t crc)
{
    m_crc = crc;
}

bool IFd::enableCheckSum()
{
    m_crc = HDLC_CRC_8;
    return true;
}

bool IFd::enableCrc16()
{
    m_crc = HDLC_CRC_16;
    return true;
}

bool IFd::enableCrc32()
{
    m_crc = HDLC_CRC_32;
    return true;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

} // namespace tinyproto
