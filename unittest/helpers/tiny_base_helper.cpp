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

#include "tiny_base_helper.h"
#include "proto/hal/tiny_types.h"

BaseHelper::BaseHelper(FakeChannel * channel,
                       int rxBufferSize)
    : m_forceStop(false)
{
    m_buffer = new uint8_t[rxBufferSize];
    m_channel = channel;
    m_thread = nullptr;
}

int BaseHelper::run(bool forked)
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

void BaseHelper::receiveThread(BaseHelper *p)
{
    while (p->m_forceStop == false)
    {
        int result = p->run();
        if ( result < 0 && result != TINY_ERR_TIMEOUT )
        {
            break;
        }
        usleep(10);
    }
}

void BaseHelper::stop()
{
    m_forceStop = true;
    if (m_thread != nullptr)
    {
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }
}

BaseHelper::~BaseHelper()
{
    delete[] m_buffer;
    m_buffer = nullptr;
}

