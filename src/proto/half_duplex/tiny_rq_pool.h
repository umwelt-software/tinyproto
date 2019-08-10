/*
    Copyright 2016-2019 (C) Alexey Dynda

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

#ifdef __cplusplus
extern "C" {
#endif

#include "proto/hal/tiny_types.h"
#include "proto/hal/tiny_list.h"

enum
{
    TINY_HD_REQUEST_INIT        = 0,
    TINY_HD_REQUEST_SUCCESSFUL  = 1,
    TINY_HD_REQUEST_DECLINED    = 2,
};

/// defines type for tiny request list item
typedef struct tiny_request_
{
    /// This is base data for tiny lists. Must be first field in the structure.
    list_element          element;
    /// state of the request
    uint8_t               state;
    /// unique id of the sent packet
    uint16_t              uid;
} tiny_request;

void tiny_put_request(tiny_request *request);

void tiny_remove_request(tiny_request *request);

void tiny_decline_requests();

void tiny_commit_request(uint16_t uid);

#ifdef __cplusplus
}
#endif

