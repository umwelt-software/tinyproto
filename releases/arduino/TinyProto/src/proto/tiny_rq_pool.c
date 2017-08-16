/*
    Copyright 2016-2017 (C) Alexey Dynda

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

#include "tiny_defines.h"
#include "tiny_list.h"
#include "tiny_rq_pool.h"
#include "tiny_hd.h"


static tiny_request * s_pool = 0;

///////////////////////////////////////////////////////////////////////////////

void tiny_put_request(tiny_request *request)
{
    request->state = TINY_HD_REQUEST_INIT;
    request->uid = tiny_list_add((list_element **)&s_pool, (list_element *)request);
}

///////////////////////////////////////////////////////////////////////////////

void tiny_remove_request(tiny_request *request)
{
    tiny_list_remove((list_element **)&s_pool, (list_element *)request);
}

///////////////////////////////////////////////////////////////////////////////

static uint8_t decline_function(list_element *element, uint16_t data)
{
    ((tiny_request *)element)->state = TINY_HD_REQUEST_DECLINED;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_decline_requests()
{
    tiny_list_enumerate((list_element *)s_pool, decline_function, 0);
    tiny_list_clear((list_element **)&s_pool);
}

///////////////////////////////////////////////////////////////////////////////

uint8_t commit_function(list_element *element, uint16_t data)
{
    if (data == ((tiny_request *)element)->uid)
    {
        ((tiny_request *)element)->state = TINY_HD_REQUEST_SUCCESSFUL;
        return 0;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_commit_request(uint16_t uid)
{
    tiny_list_enumerate((list_element *)s_pool, commit_function, uid);
}

///////////////////////////////////////////////////////////////////////////////

