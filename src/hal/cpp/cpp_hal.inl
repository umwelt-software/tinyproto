/*
    Copyright 2022 (C) Alexey Dynda

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

#include <mutex>

#error "Not implemented now"

void tiny_mutex_create(tiny_mutex_t *mutex)
{
}

void tiny_mutex_destroy(tiny_mutex_t *mutex)
{
}

void tiny_mutex_lock(tiny_mutex_t *mutex)
{
}

uint8_t tiny_mutex_try_lock(tiny_mutex_t *mutex)
{
}

void tiny_mutex_unlock(tiny_mutex_t *mutex)
{
}

void tiny_events_create(tiny_events_t *events)
{
}

void tiny_events_destroy(tiny_events_t *events)
{
}

uint8_t tiny_events_wait(tiny_events_t *events, uint8_t bits, uint8_t clear, uint32_t timeout)
{
}

uint8_t tiny_events_check_int(tiny_events_t *events, uint8_t bits, uint8_t clear)
{
}

void tiny_events_set(tiny_events_t *events, uint8_t bits)
{
}

void tiny_events_clear(tiny_events_t *events, uint8_t bits)
{
}

void tiny_sleep(uint32_t ms)
{
}

void tiny_sleep_us(uint32_t us)
{
}

uint32_t tiny_millis(void)
{
}

uint32_t tiny_micros(void)
{
}

