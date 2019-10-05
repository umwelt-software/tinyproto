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

#pragma once

#include "fake_wire.h"
#include "fake_channel.h"

class FakeConnection
{
public:
    FakeConnection()=default;
    ~FakeConnection()=default;

    FakeChannel& endpoint1() { return m_endpoint1; }
    FakeChannel& endpoint2() { return m_endpoint2; }

    FakeWire& line1() { return m_line1; }
    FakeWire& line2() { return m_line2; }
private:
    FakeWire  m_line1{};
    FakeWire  m_line2{};
    FakeChannel m_endpoint1{&m_line1, &m_line2};
    FakeChannel m_endpoint2{&m_line2, &m_line1};
};
