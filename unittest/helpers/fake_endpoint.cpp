/*
    Copyright 2017-2020 (C) Alexey Dynda

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

#include "fake_endpoint.h"

FakeEndpoint::FakeEndpoint(FakeWire &tx, FakeWire &rx, int rxSize, int txSize)
    : m_tx(tx)
    , m_rx(rx)
{
    m_txBlock = m_tx.CreateTxHardware(txSize);
    m_rxBlock = m_rx.CreateRxHardware(rxSize);
}

FakeEndpoint::~FakeEndpoint()
{
}

int FakeEndpoint::read(uint8_t *data, int length)
{
    return m_rxBlock->Read(data, length, m_timeout);
}

int FakeEndpoint::write(const uint8_t *data, int length)
{
    return m_txBlock->Write(data, length, m_timeout);
}

bool FakeEndpoint::wait_until_rx_count(int count, int timeout)
{
    return m_rxBlock->WaitUntilRxCount(count, timeout);
}
