/*
    Copyright 2019-2020,2022 (C) Alexey Dynda

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

#include "tiny_base_helper.h"
#include "hal/tiny_types.h"

BaseHelper::BaseHelper(FakeEndpoint *endpoint, int rxBufferSize)
    : m_forceStop(false)
{
    m_bufferOriginPtr = new uint8_t[rxBufferSize + TINY_ALIGN_STRUCT_VALUE - 1];
    m_buffer = (uint8_t *)( ((uintptr_t)m_bufferOriginPtr + TINY_ALIGN_STRUCT_VALUE - 1) & (~(TINY_ALIGN_STRUCT_VALUE-1)) );
    m_endpoint = endpoint;
}

int BaseHelper::run(bool forked)
{
    if ( forked )
    {
        m_forceStop = false;
        m_receiveThread = new std::thread(receiveThread, this);
        m_sendThread = new std::thread(sendThread, this);
        return 0;
    }
    else
    {
        return run_rx() | run_tx();
    }
}

void BaseHelper::receiveThread(BaseHelper *p)
{
    while ( p->m_forceStop == false )
    {
        int result = p->run_rx();
        if ( result < 0 && result != TINY_ERR_TIMEOUT && result != TINY_ERR_WRONG_CRC )
        {
            break;
        }
    }
}

void BaseHelper::sendThread(BaseHelper *p)
{
    while ( p->m_forceStop == false )
    {
        int result = p->run_tx();
        if ( result < 0 && result != TINY_ERR_TIMEOUT )
        {
            break;
        }
    }
}

void BaseHelper::stop()
{
    m_forceStop = true;
    if ( m_receiveThread != nullptr )
    {
        m_receiveThread->join();
        delete m_receiveThread;
        m_receiveThread = nullptr;
    }
    if ( m_sendThread != nullptr )
    {
        m_sendThread->join();
        delete m_sendThread;
        m_sendThread = nullptr;
    }
}

BaseHelper::~BaseHelper()
{
    delete[] m_bufferOriginPtr;
    m_bufferOriginPtr = nullptr;
}
