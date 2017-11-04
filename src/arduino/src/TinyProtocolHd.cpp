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

#if defined(ARDUINO)
#   if ARDUINO >= 100
    #include "Arduino.h"
#   else
    #include "WProgram.h"
#   endif
#endif

#include "TinyProtocolHd.h"

namespace Tiny
{
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void ProtoHd::onReceiveInternal(void *handle, uint16_t uid, uint8_t *pdata, int size)
{
    (reinterpret_cast<ProtoHd*>(handle))->m_onReceive(pdata, size);
}

void ProtoHd::begin(write_block_cb_t writecb,
                    read_block_cb_t readcb)
{
    STinyHdInit   init = {0};
    init.write_func       = writecb;
    init.read_func        = readcb;
    init.pdata            = this;
    init.on_frame_cb      = onReceiveInternal;
    init.inbuf            = m_buffer;
    init.inbuf_size       = m_bufferSize;
    init.timeout          = 1000;
    init.multithread_mode = 0;

    tiny_hd_init( &m_data, &init  );
}

void ProtoHd::end()
{
    tiny_hd_close(&m_data);
}

int ProtoHd::write(char* buf, int size)
{
    return tiny_send_wait_ack(&m_data, buf, size);
}

int ProtoHd::write(Packet &pkt)
{
    return tiny_send_wait_ack(&m_data, pkt.m_buf, pkt.m_len);
}

int ProtoHd::run()
{
    return tiny_hd_run(&m_data);
}

void ProtoHd::disableCrc()
{
    tiny_set_fcs_bits(&m_data.handle, 0);
}

bool ProtoHd::enableCheckSum()
{
    return tiny_set_fcs_bits(&m_data.handle, 8) == TINY_NO_ERROR;
}

bool ProtoHd::enableCrc16()
{
    return tiny_set_fcs_bits(&m_data.handle, 16) == TINY_NO_ERROR;
}

bool ProtoHd::enableCrc32()
{
    return tiny_set_fcs_bits(&m_data.handle, 32) == TINY_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

}  // namespace Tiny

