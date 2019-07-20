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
#pragma once

#include <functional>
#include <stdint.h>
#include "fake_channel.h"
#include "hdlc/tiny_hdlc.h"

class TinyHdlcHelper
{
private:
    hdlc_struct_t     m_handle;
public:
    TinyHdlcHelper(FakeChannel         * channel,
                   const std::function<void(uint8_t*,int)> &onRxFrameCb = nullptr,
                   const std::function<void(uint8_t*,int)> &onTxFrameCb = nullptr,
                   int rx_buf_size = 1024 );
    ~TinyHdlcHelper();
    int send(uint8_t *buf, int len);
    int process_rx_bytes();
private:
    static uint32_t s_handleOffset;
    FakeChannel * m_channel;
    std::function<void(uint8_t*,int)> m_onRxFrameCb;
    std::function<void(uint8_t*,int)> m_onTxFrameCb;

    static int    onRxFrame(void *handle, void * buf, int len);
    static int    onTxFrame(void *handle, const void * buf, int len);
    static int    read_data(void * appdata, uint8_t * data, int length);
    static int    write_data(void * appdata, const void * data, int length);
};

