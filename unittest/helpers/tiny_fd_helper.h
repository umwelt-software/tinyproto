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

#pragma once

#include "tiny_base_helper.h"
#include <functional>
#include <stdint.h>
#include <thread>
#include <atomic>
#include "proto/fd/tiny_fd.h"
#include "fake_channel.h"


class TinyHelperFd: public IBaseHelper<TinyHelperFd>
{
public:
    TinyHelperFd(FakeChannel * channel,
                 int rxBufferSize,
                 const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb = nullptr,
                 bool  multithread_mode = false);
    virtual ~TinyHelperFd();
    int send(uint8_t *buf, int len, int timeout = 30);
    int run_rx() override;
    int run_tx() override;
    using IBaseHelper<TinyHelperFd>::run;
private:
    tiny_fd_handle_t   m_handle;
    std::function<void(uint16_t,uint8_t*,int)>
                  m_onRxFrameCb;

    static void   onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len);
};

