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

#include "tiny_base_helper.h"
#include <functional>
#include <stdint.h>
#include <thread>
#include "fake_channel.h"
#include "proto/hdlc/tiny_hdlc.h"

class TinyHdlcHelper: public IBaseHelper<TinyHdlcHelper>
{
private:
    hdlc_struct_t     m_handle;
public:
    TinyHdlcHelper(FakeChannel         * channel,
                   const std::function<void(uint8_t*,int)> &onRxFrameCb = nullptr,
                   const std::function<void(uint8_t*,int)> &onTxFrameCb = nullptr,
                   int rx_buf_size = 1024,
                   hdlc_crc_t crc = HDLC_CRC_16);
    ~TinyHdlcHelper();
    int send(uint8_t *buf, int len, int timeout = 1000);
    void send(int count, const std::string &msg);
    int run_rx() override;
    int run_rx_until_read(int timeout = 1000);
    int run_tx() override;
    int rx_count() { return m_rx_count; }
    int tx_count() { return m_tx_count; }

    void setMcuMode() { m_tx_from_main = true; }
private:
    std::function<void(uint8_t*,int)> m_onRxFrameCb;
    std::function<void(uint8_t*,int)> m_onTxFrameCb;
    bool m_stop_sender = false;
    std::thread * m_sender_thread = nullptr;
    int m_rx_count = 0;
    int m_tx_count = 0;
    bool m_tx_from_main = false;

    static int    onRxFrame(void *handle, void * buf, int len);
    static int    onTxFrame(void *handle, const void * buf, int len);
    static void   MessageSenderStatic( TinyHdlcHelper * helper, int count, std::string msg );
    void MessageSender( int count, std::string msg );
};

