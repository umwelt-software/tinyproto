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

#include "tiny_hd_helper.h"


uint32_t TinyHelperHd::s_handleOffset;

TinyHelperHd::TinyHelperHd(FakeChannel * channel,
                           int rxBufferSize,
                           const std::function<void(uint16_t,uint8_t*,int)> &onRxFrameCb,
                           bool  multithread_mode)
    :m_onRxFrameCb(onRxFrameCb)
    ,m_forceStop(false)
{
    s_handleOffset = (uint8_t *)this - (uint8_t *)(&m_handle);
    m_buffer = new uint8_t[rxBufferSize];
    m_channel = channel;
    m_thread = nullptr;
    STinyHdInit   init = {0};
    init.write_func       = TinyHelperHd::write_data;
    init.read_func        = TinyHelperHd::read_data;
    init.pdata            = this;
    init.on_frame_cb      = onRxFrame;
    init.inbuf            = m_buffer;
    init.inbuf_size       = rxBufferSize;
    init.timeout          = 2000;
    init.multithread_mode = multithread_mode;

    tiny_hd_init( &m_handle, &init  );
    tiny_set_fcs_bits( &m_handle.handle, 16);
}


int TinyHelperHd::send_wait_ack(uint8_t *buf, int len)
{
    return tiny_send_wait_ack(&m_handle, buf, len);
}


int TinyHelperHd::run()
{
    return tiny_hd_run(&m_handle);
}


int TinyHelperHd::run(bool forked)
{
    if (forked)
    {
        m_thread = new std::thread(receiveThread,this);
        return 0;
    }
    else
    {
        return run();
    }
}


void  TinyHelperHd::onRxFrame(void *handle, uint16_t uid, uint8_t * buf, int len)
{
    TinyHelperHd * helper = reinterpret_cast<TinyHelperHd *>( ((uint8_t *)handle) - s_handleOffset );
    if (helper->m_onRxFrameCb)
    {
        helper->m_onRxFrameCb(uid, buf, len);
    }
}


int TinyHelperHd::read_data(void * appdata, uint8_t * data, int length)
{
    TinyHelperHd  *helper = reinterpret_cast<TinyHelperHd *>(appdata);
    return helper->m_channel->read(data, length);
}


int TinyHelperHd::write_data(void * appdata, const uint8_t * data, int length)
{
    TinyHelperHd *helper = reinterpret_cast<TinyHelperHd *>(appdata);
    return helper->m_channel->write(data, length);
}


void TinyHelperHd::receiveThread(TinyHelperHd *p)
{
    while (p->m_forceStop == false)
    {
        int result = p->run();
        if (result <0)
        {
            break;
        }
        usleep(10);
    }
}


TinyHelperHd::~TinyHelperHd()
{
    if (m_thread != nullptr)
    {
        m_forceStop = true;
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }
    tiny_hd_close( &m_handle );
    delete[] m_buffer;
    m_buffer = nullptr;
}

