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

#ifndef _FAKE_CHANNEL_H_
#define _FAKE_CHANNEL_H_

#include <stdint.h>
#include "fake_wire.h"


class FakeChannel
{
public:
    FakeChannel(FakeWire *tx, FakeWire *rx);
    ~FakeChannel();
    int read(uint8_t * data, int length);
    int write(const uint8_t * data, int length);
private:
    FakeWire * m_tx;
    FakeWire * m_rx;
    std::mutex m_mutex;
};


#endif /* _FAKE_WIRE_H_ */
