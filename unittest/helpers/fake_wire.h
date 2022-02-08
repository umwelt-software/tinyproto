/*
    Copyright 2017,2020,2022 (C) Alexey Dynda

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

#include <cstdint>
#include <mutex>
#include <list>
#include <queue>
#include <thread>
#include <atomic>

#include "semaphore_helper.h"

class TxHardwareBlock
{
public:
    TxHardwareBlock(int size);
    ~TxHardwareBlock();

    int Write(const void *buffer, int size, uint32_t timeout = 10);

    void Disable()
    {
        m_enabled = false;
    }

    void Enable()
    {
        m_enabled = true;
    }

private:
    int m_size = 0;
    bool m_enabled = true;
    std::queue<uint8_t> m_queue{};
    tiny_events_t m_events;
    tiny_mutex_t m_mutex;

    int Peek(uint8_t &byte);

    friend class FakeWire;
};

class RxHardwareBlock
{
public:
    RxHardwareBlock(int size);
    ~RxHardwareBlock();

    int Read(void *buffer, int size, uint32_t timeout = 10);

    void Disable()
    {
        m_enabled = false;
    }

    void Enable()
    {
        m_enabled = true;
    }

    bool WaitUntilRxCount(int count, uint32_t timeout);

    void Flush();

private:
    int m_size = 0;
    bool m_enabled = true;
    std::queue<uint8_t> m_queue{};
    tiny_events_t m_events;
    tiny_mutex_t m_mutex;

    int Put(uint8_t byte);

    friend class FakeWire;
};

class FakeWire
{
public:
    FakeWire();

    ~FakeWire();

    RxHardwareBlock *GetRx() { return m_rx.front(); }

    TxHardwareBlock *GetTx() { return m_tx.front(); }

    RxHardwareBlock *CreateRxHardware(int size = 1024);

    TxHardwareBlock *CreateTxHardware(int size = 1024);

    bool wait_until_rx_count(int count, int timeout);

    void generate_error_every_n_byte(int n)
    {
        m_errors.push_back(ErrorDesc{0, n, -1});
    };

    void generate_single_error(int position)
    {
        m_errors.push_back(ErrorDesc{position, position, 1});
    };

    void generate_error(int first, int period, int count = -1)
    {
        m_errors.push_back(ErrorDesc{first, period, count});
    };

    int lostBytes()
    {
        return m_lostBytes;
    }

private:
    typedef struct
    {
        int first;
        int period;
        int count;
    } ErrorDesc;
    std::list<TxHardwareBlock *> m_tx{};
    std::list<RxHardwareBlock *> m_rx{};
    std::list<ErrorDesc> m_errors;
    int  cnt = 0;
    int m_byte_counter = 0;
    bool m_enabled = true;
    std::atomic<int> m_lostBytes{0};

    void TransferData(int num_bytes);

    friend class FakeConnection;
};
