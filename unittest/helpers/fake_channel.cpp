/*
    Copyright 2017-2019 (C) Alexey Dynda

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

#include "fake_channel.h"


FakeChannel::FakeChannel(FakeWire *tx, FakeWire *rx)
{
    m_rx = rx;
    m_tx = tx;
}

FakeChannel::~FakeChannel()
{
}


int FakeChannel::read(uint8_t * data, int length)
{
    return m_rx->read(data, length, m_timeout);
}


int FakeChannel::write(const uint8_t * data, int length)
{
    return m_tx->write(data, length, m_timeout);
}


