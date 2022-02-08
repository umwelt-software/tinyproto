/*
    Copyright 2019-2020,2022 (C) Alexey Dynda

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

#if defined(ARDUINO)
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#endif

#include "TinyProtocolHdlc.h"

namespace tinyproto
{
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

int Hdlc::onReceiveInternal(void *handle, void *pdata, int size)
{
    (reinterpret_cast<Hdlc *>(handle))->onReceive((uint8_t *)pdata, size);
    return 0;
}

int Hdlc::onSendInternal(void *handle, const void *pdata, int size)
{
    (reinterpret_cast<Hdlc *>(handle))->onSend((const uint8_t *)pdata, size);
    return 0;
}

void Hdlc::begin(write_block_cb_t writecb, read_block_cb_t readcb)
{
    m_data.send_tx = writecb;
    m_data.on_frame_read = onReceiveInternal;
    m_data.on_frame_sent = onSendInternal;
    m_data.rx_buf = m_buffer;
    m_data.rx_buf_size = m_bufferSize;
    m_data.crc_type = m_crc;
    m_data.multithread_mode = false;
    m_data.user_data = this;

    m_handle = hdlc_init(&m_data);
}

void Hdlc::begin()
{
    begin(nullptr, nullptr);
}

void Hdlc::end()
{
    if ( m_bufferSize == 0 )
        return;
    hdlc_close(m_handle);
}

int Hdlc::write(const char *buf, int size)
{
    return hdlc_send(m_handle, buf, size, 0);
}

int Hdlc::write(const IPacket &pkt)
{
    return write((char *)pkt.m_buf, pkt.m_len);
}

int Hdlc::run_rx(const void *data, int len)
{
    return hdlc_run_rx(m_handle, data, len, nullptr);
}

int Hdlc::run_tx(void *data, int max_len)
{
    return hdlc_get_tx_data(m_handle, data, max_len);
}

void Hdlc::disableCrc()
{
    m_crc = HDLC_CRC_OFF;
}

void Hdlc::enableCrc(hdlc_crc_t crc)
{
    m_crc = crc;
}

bool Hdlc::enableCheckSum()
{
    m_crc = HDLC_CRC_8;
    return true;
}

bool Hdlc::enableCrc16()
{
    m_crc = HDLC_CRC_16;
    return true;
}

bool Hdlc::enableCrc32()
{
    m_crc = HDLC_CRC_32;
    return true;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

} // namespace tinyproto
