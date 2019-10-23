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

#include "fake_connection.h"
#include <thread>
#include <chrono>

void FakeConnection::TransferDataStatic(FakeConnection *conn)
{
    conn->TransferData();
}

void FakeConnection::TransferData()
{
    auto startTs = std::chrono::steady_clock::now();
    while (!m_stopped)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(m_interval_us));
        int bytes = 0;
        auto endTs = std::chrono::steady_clock::now();
        while ( startTs < endTs )
        {
            startTs +=  std::chrono::microseconds( m_interval_us.load() ? : 1);
            bytes++;
        }
        m_line1.TransferData( bytes );
        m_line2.TransferData( bytes );
    }
}

