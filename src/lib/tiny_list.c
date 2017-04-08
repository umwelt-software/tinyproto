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


#ifdef PLATFORM_MUTEX
static PLATFORM_MUTEX s_mutex;
#endif

static uint16_t       s_uid = 0;

///////////////////////////////////////////////////////////////////////////////

uint16_t tiny_list_add(list_element **head, list_element *element)
{
    uint16_t uid;
    MUTEX_LOCK(s_mutex);

    uid = s_uid++;
    element->pnext = *head;
    element->pprev = 0;
    if (*head)
    {
        (*head)->pprev = element;
    }
    *head = element;

    MUTEX_UNLOCK(s_mutex);
    return uid & 0x0FFF;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_list_remove(list_element **head, list_element *element)
{
    MUTEX_LOCK(s_mutex);
    if (element == *head)
    {
        *head = element->pnext;
        if (*head)
        {
            (*head)->pprev = 0;
        }
    }
    else
    {
        if (element->pprev)
        {
            element->pprev->pnext = element->pnext;
        }
        if (element->pnext)
        {
            element->pnext->pprev = element->pprev;
        }
    }
    element->pprev = 0;
    element->pnext = 0;
    MUTEX_UNLOCK(s_mutex);
}

///////////////////////////////////////////////////////////////////////////////

void tiny_list_clear(list_element **head)
{
    MUTEX_LOCK(s_mutex);
    *head = 0;
    MUTEX_UNLOCK(s_mutex);
}

///////////////////////////////////////////////////////////////////////////////

void tiny_list_enumerate(list_element *head, uint8_t (*enumerate_func)(list_element *element))
{
    MUTEX_LOCK(s_mutex);
    while (head)
    {
        if (!enumerate_func(head))
        {
            break;
        }
        head = head->pnext;
    }
    MUTEX_UNLOCK(s_mutex);
}

///////////////////////////////////////////////////////////////////////////////

