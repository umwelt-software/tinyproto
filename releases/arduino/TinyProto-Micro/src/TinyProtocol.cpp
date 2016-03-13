/*
    Copyright 2016 (C) Alexey Dynda

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

#include "TinyProtocol.h"

namespace Tiny {
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void Proto::begin(write_block_cb_t writecb,
                  read_block_cb_t readcb)
{
    tiny_init(&m_data, writecb, readcb);
}

void Proto::end()
{
    tiny_close(&m_data);
}

int Proto::write(char* buf, int size, uint8_t flags)
{
    return tiny_send(&m_data, 0, (uint8_t*)buf, size, flags); 
}

int Proto::read(char *buf, int size, uint8_t flags)
{
    return tiny_read(&m_data, 0, (uint8_t*)buf, size, flags);
}

int Proto::write(Packet &pkt, uint8_t flags)
{
    return tiny_send(&m_data, m_uidEnabled ? &pkt.m_uid : 0, pkt.m_buf, pkt.m_len, flags) > 0;
}

int Proto::read(Packet &pkt, uint8_t flags)
{
    int len = tiny_read(&m_data, m_uidEnabled ? &pkt.m_uid : 0, pkt.m_buf, pkt.m_size, flags);
    pkt.m_p = 0;
    pkt.m_len = len;
    return len;
}

void Proto::disableCrc()
{
    tiny_set_fcs_bits(&m_data, 0);
}

bool Proto::enableCheckSum()
{
    return tiny_set_fcs_bits(&m_data, 8) == TINY_NO_ERROR;
}

bool Proto::enableCrc16()
{
    return tiny_set_fcs_bits(&m_data, 16) == TINY_NO_ERROR;
}

bool Proto::enableCrc32()
{
    return tiny_set_fcs_bits(&m_data, 32) == TINY_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

}  // namespace Tiny

