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

#include "TinyHdlc.h"

namespace Tiny
{
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void IHdlc::begin(write_block_cb_t writecb,
                  read_block_cb_t readcb)
{
    m_hdlc->crc_type = m_crc;
    m_hdlc->send_tx = writecb;
    m_hdlc->user_data = this;
    m_readcb = readcb;
    hdlc_init(m_hdlc);
}

void IHdlc::end()
{
    hdlc_close(m_hdlc);
}

int IHdlc::runTx()
{
    return hdlc_run_tx(m_hdlc);
}

int IHdlc::runRx(const char *buf, int size)
{
    return hdlc_run_rx(m_hdlc, buf, size, nullptr);
}

int IHdlc::runRx()
{
    int result = -1;
    if ( m_readcb )
    {
        uint8_t data;
        result = m_readcb(this, &data, sizeof(data));
        if ( result > 0 )
        {
            result = hdlc_run_rx( m_hdlc, &data, result, nullptr );
        }
    }
    return result;
}

int IHdlc::write(const char* buf, int size)
{
    return hdlc_send( m_hdlc, (const char *)buf, size, 1000 );
}

int IHdlc::read(char *buf, int size)
{
    hdlc_set_rx_buffer( m_hdlc, buf, size );
    return hdlc_run_rx_until_read( m_hdlc, m_readcb, this, 1000 );
}

int IHdlc::write(IPacket &pkt)
{
    return write( (const char *)pkt.m_buf, pkt.m_len );
}

int IHdlc::read(IPacket &pkt)
{
    int len = read( (char *)pkt.m_buf, pkt.m_size );
    pkt.m_p = 0;
    pkt.m_len = len;
    return len;
}

void IHdlc::disableCrc()
{
    m_crc = HDLC_CRC_OFF;
}

bool IHdlc::enableCheckSum()
{
#if defined(CONFIG_ENABLE_FCS8)
    m_crc = HDLC_CRC_8;
    return true;
#else
    return false;
#endif
}

bool IHdlc::enableCrc16()
{
#if defined(CONFIG_ENABLE_FCS16)
    m_crc = HDLC_CRC_16;
    return true;
#else
    return false;
#endif
}

bool IHdlc::enableCrc32()
{
#if defined(CONFIG_ENABLE_FCS32)
    m_crc = HDLC_CRC_32;
    return true;
#else
    return false;
#endif
}

#ifdef ARDUINO

static int writeToSerial(void *p, const void *b, int s)
{
    int result = Serial.write((const uint8_t *)b, s);
    return result;
}

static int readFromSerial(void *p, void *b, int s)
{
    int length = Serial.readBytes((uint8_t *)b, s);
    return length;
}


void IHdlc::beginToSerial()
{
    Serial.setTimeout( 100 );
    begin(writeToSerial, readFromSerial);
}
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

}  // namespace Tiny

