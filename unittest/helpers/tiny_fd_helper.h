/*
    Copyright 2017-2022 (C) Alexey Dynda

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

#pragma once

#include "tiny_base_helper.h"
#include <functional>
#include <stdint.h>
#include <thread>
#include <atomic>
#include "proto/fd/tiny_fd.h"
#include "fake_endpoint.h"

class TinyHelperFd: public IBaseHelper<TinyHelperFd>
{
public:
    TinyHelperFd(FakeEndpoint *endpoint, int rxBufferSize,
                 const std::function<void(uint8_t *, int)> &onRxFrameCb = nullptr, int window_frames = 7,
                 int timeout = -1);
    virtual ~TinyHelperFd();
    int send(uint8_t *buf, int len);
    int send(const std::string &message);
    int send(int count, const std::string &msg);
    int run_rx() override;
    int run_tx() override;
    void set_ka_timeout(uint32_t timeout)
    {
        tiny_fd_set_ka_timeout(m_handle, timeout);
    }
    using IBaseHelper<TinyHelperFd>::run;

    void wait_until_rx_count(int count, uint32_t timeout);

    int rx_count()
    {
        return m_rx_count;
    }
    int tx_count()
    {
        return m_tx_count;
    }

    void set_connect_cb(const std::function<void(uint8_t, bool)> &onConnectCb);

private:
    tiny_fd_handle_t m_handle;
    int m_rx_count = 0;
    int m_tx_count = 0;
    std::thread *m_message_sender = nullptr;
    std::function<void(uint8_t *, int)> m_onRxFrameCb;
    std::function<void(uint8_t, bool)> m_onConnectCb = nullptr;
    bool m_stop_sender = false;

    static void onRxFrame(void *handle, uint8_t *buf, int len);
    static void onTxFrame(void *handle, uint8_t *buf, int len);
    static void onConnect(void *handle, uint8_t addr, bool connected);
    static void MessageSender(TinyHelperFd *helper, int count, std::string message);
};
