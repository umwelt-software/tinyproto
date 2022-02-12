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

#include "fake_connection.h"
#include <thread>
#include <chrono>

FakeSetup::FakeSetup()
{
    setSpeed( 512000 );
    m_endpoints.push_back(new FakeEndpoint(m_line1, m_line2, 256, 256));
    m_endpoints.push_back(new FakeEndpoint(m_line2, m_line1, 256, 256));
    m_line_thread = new std::thread(TransferDataStatic, this);
}

FakeSetup::FakeSetup(int p1_hw_size, int p2_hw_size)
   : m_line1()
   , m_line2()
{
    setSpeed( 512000 );
    m_endpoints.push_back(new FakeEndpoint(m_line1, m_line2, p1_hw_size, p1_hw_size));
    m_endpoints.push_back(new FakeEndpoint(m_line2, m_line1, p2_hw_size, p2_hw_size));
    m_line_thread = new std::thread(TransferDataStatic, this);
}

FakeSetup::~FakeSetup()
{
    m_stopped = true;
    m_line_thread->join();
    delete m_line_thread;
    for (auto e: m_endpoints)
    {
        delete e;
    }
}

void FakeSetup::TransferDataStatic(FakeSetup *conn)
{
    conn->TransferData();
}

void FakeSetup::TransferData()
{
    auto startTs = std::chrono::steady_clock::now();
    while ( !m_stopped )
    {
        std::this_thread::sleep_for(std::chrono::microseconds(m_interval_us));
        auto endTs = std::chrono::steady_clock::now();
        uint64_t duration = std::chrono::duration_cast<std::chrono::microseconds>(endTs - startTs).count();
        uint32_t bytes = m_Bps * duration / 1000000ULL;
        if ( bytes == 0 )
        {
            bytes = 1;
        }
        startTs = endTs;
        m_line1.TransferData(bytes);
        m_line2.TransferData(bytes);
    }
}

FakeEndpoint &FakeSetup::endpoint1()
{
    auto it = m_endpoints.begin();
    return **it;
}

FakeEndpoint &FakeSetup::endpoint2()
{
    auto it = m_endpoints.begin();
    it++;
    return **it;
}

void FakeSetup::setSpeed(uint32_t bps)
{
    uint64_t Bps = bps / 8;
    uint64_t interval = 1000000ULL / (Bps);
    interval = interval < 50 ? 50: interval;
    m_Bps = Bps;
    m_interval_us = interval;
}

int FakeSetup::lostBytes()
{
    return m_line1.lostBytes() + m_line2.lostBytes();
}
