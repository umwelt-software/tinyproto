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
#pragma once

#include <functional>
#include <stdint.h>
#include "fake_endpoint.h"
#include "proto/light/tiny_light.h"

class TinyLightHelper
{
private:
    STinyLightData m_handle;

public:
    TinyLightHelper(FakeEndpoint *endpoint, int rx_buf_size = 1024);
    ~TinyLightHelper();
    int send(uint8_t *buf, int len);
    int read(uint8_t *buf, int len);

private:
    static uint32_t s_handleOffset;
    FakeEndpoint *m_endpoint;

    static int read_data(void *appdata, void *data, int length);
    static int write_data(void *appdata, const void *data, int length);
};
