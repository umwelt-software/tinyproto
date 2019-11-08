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

#if defined(ARDUINO)
#   if ARDUINO >= 100
    #include "Arduino.h"
#   else
    #include "WProgram.h"
#   endif
#endif

#include "TinyLightProtocol.h"

namespace Tiny
{
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void ProtoLight::begin(write_block_cb_t writecb,
                  read_block_cb_t readcb)
{
    m_data.crc_type = m_crc;
    tiny_light_init(&m_data, writecb, readcb, this);
}

void ProtoLight::end()
{
    tiny_light_close(&m_data);
}

int ProtoLight::write(char* buf, int size)
{
    return tiny_light_send(&m_data, (uint8_t*)buf, size);
}

int ProtoLight::read(char *buf, int size)
{
    return tiny_light_read(&m_data, (uint8_t*)buf, size);
}

int ProtoLight::write(IPacket &pkt)
{
    return tiny_light_send(&m_data, pkt.m_buf, pkt.m_len) > 0;
}

int ProtoLight::read(IPacket &pkt)
{
    int len = tiny_light_read(&m_data, pkt.m_buf, pkt.m_size);
    pkt.m_p = 0;
    pkt.m_len = len;
    return len;
}

void ProtoLight::disableCrc()
{
    m_crc = HDLC_CRC_OFF;
}

void ProtoLight::enableCrc(hdlc_crc_t crc)
{
    m_crc = crc;
}

bool ProtoLight::enableCheckSum()
{
#if defined(CONFIG_ENABLE_CHECKSUM)
    m_crc = HDLC_CRC_8;
    return true;
#else
    return false;
#endif
}

bool ProtoLight::enableCrc16()
{
#if defined(CONFIG_ENABLE_FCS16)
    m_crc = HDLC_CRC_16;
    return true;
#else
    return false;
#endif
}

bool ProtoLight::enableCrc32()
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


void ProtoLight::beginToSerial()
{
    Serial.setTimeout( 100 );
    begin(writeToSerial, readFromSerial);
}
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

}  // namespace Tiny

