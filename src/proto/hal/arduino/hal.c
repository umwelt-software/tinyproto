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

#if !defined(__AVR__) && !defined(__XTENSA__) && defined(ARDUINO)

#include "proto/hal/tiny_types.h"

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
    return 1;
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

uint8_t tiny_events_wait_and_clear(tiny_events_t *events, uint8_t bits)
{
    return bits;
}

uint8_t tiny_events_wait(tiny_events_t *events, uint8_t bits)
{
    return bits;
}

void tiny_events_set(tiny_events_t *events, uint8_t bits)
{
}

void tiny_events_clear(tiny_events_t *events, uint8_t bits)
{
}

void tiny_sleep(uint32_t ms)
{
    delay(ms);
}

uint32_t tiny_millis()
{
    return millis();
}

#endif
