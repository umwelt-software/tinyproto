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

#ifndef _TINY_HD_HELPER_H_
#define _TINY_HD_HELPER_H_

#include "tiny_base_helper.h"
#include <functional>
#include <stdint.h>
#include <thread>
#include <atomic>
#include "proto/_old/tiny_layer2.h"
#include "proto/half_duplex/tiny_hd.h"
#include "fake_channel.h"


class TinyHelperHd: public IBaseHelper<TinyHelperHd>
{
public:
    TinyHelperHd(FakeChannel * channel,
                 int rxBufferSize,
                 const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb = nullptr,
                 bool  multithread_mode = false);
    ~TinyHelperHd();
    int send_wait_ack(uint8_t *buf, int len);
    int run() override;
    using IBaseHelper<TinyHelperHd>::run;
private:
    STinyHdData   m_handle;
    std::function<void(uint16_t,uint8_t*,int)>
                  m_onRxFrameCb;

    static void   onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len);
};

#endif /* _FAKE_WIRE_H_ */
