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

#if defined(ARDUINO)
#   if ARDUINO >= 100
    #include "Arduino.h"
#   else
    #include "WProgram.h"
#   endif
#endif

#include "TinyProtocolFd.h"

namespace Tiny
{
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void IProtoFd::onReceiveInternal(void *handle, uint16_t uid, uint8_t *pdata, int size)
{
    (reinterpret_cast<IProtoFd*>(handle))->onReceive(pdata, size);
}

void IProtoFd::begin(write_block_cb_t writecb,
                     read_block_cb_t readcb)
{
    tiny_fd_init_t        init{};
    init.write_func       = writecb;
    init.read_func        = readcb;
    init.pdata            = this;
    init.on_frame_cb      = onReceiveInternal;
//    init.on_sent_cb       = onTxFrame;
    init.buffer           = m_buffer;
    init.buffer_size      = m_bufferSize;
    init.window_frames    = m_window;
    init.send_timeout     = m_sendTimeout;
    init.retry_timeout    = 200;
    init.retries          = 2;
    init.crc_type         = m_crc;

    tiny_fd_init( &m_handle, &init  );
}

void IProtoFd::end()
{
    if ( m_bufferSize == 0 ) return;
    tiny_fd_close( m_handle );
}

int IProtoFd::write(char* buf, int size)
{
    return tiny_fd_send( m_handle, buf, size );
}

int IProtoFd::write(IPacket &pkt)
{
    return tiny_fd_send( m_handle, pkt.m_buf, pkt.m_len );
}

int IProtoFd::run_rx(uint16_t timeout)
{
    return tiny_fd_run_rx( m_handle, timeout );
}

int IProtoFd::run_tx(uint16_t timeout)
{
    return tiny_fd_run_tx( m_handle, timeout );
}

void IProtoFd::disableCrc()
{
    m_crc = HDLC_CRC_OFF;
}

void IProtoFd::enableCrc(hdlc_crc_t crc)
{
    m_crc = crc;
}

bool IProtoFd::enableCheckSum()
{
    m_crc = HDLC_CRC_8;
    return true;
}

bool IProtoFd::enableCrc16()
{
    m_crc = HDLC_CRC_16;
    return true;
}

bool IProtoFd::enableCrc32()
{
    m_crc = HDLC_CRC_32;
    return true;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

}  // namespace Tiny

