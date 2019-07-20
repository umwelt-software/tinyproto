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

#include <functional>
#include <stdint.h>
#include <thread>
#include <atomic>
#include "tiny_layer2.h"
#include "tiny_hd.h"
#include "fake_channel.h"


class TinyHelperHd
{
private:
    STinyHdData   m_handle;
public:
    TinyHelperHd(FakeChannel * channel,
                 int rxBufferSize,
                 const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb = nullptr,
                 bool  multithread_mode = false);
    ~TinyHelperHd();
    int send_wait_ack(uint8_t *buf, int len);
    int run();
    int run(bool forked);
private:
    static uint32_t s_handleOffset;
    FakeChannel * m_channel;
    std::function<void(uint16_t,uint8_t*,int)>
                  m_onRxFrameCb;
    std::thread * m_thread;
    std::atomic<bool>
                  m_forceStop;
    uint8_t     * m_buffer;

    static void   onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len);
    static int    read_data(void * appdata, void * data, int length);
    static int    write_data(void * appdata, const void * data, int length);
    static void   receiveThread(TinyHelperHd *p);
};

#endif /* _FAKE_WIRE_H_ */
