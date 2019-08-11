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

#if defined(__AVR__)

#if defined(ARDUINO)
#include <Arduino.h>
#endif
// TODO: <util/atomic.h>

#include "proto/hal/tiny_types.h"
#include <avr/interrupt.h>
#include <util/delay.h>

void tiny_mutex_create(tiny_mutex_t *mutex)
{
    *mutex = 0;
}

void tiny_mutex_destroy(tiny_mutex_t *mutex)
{
}

void tiny_mutex_lock(tiny_mutex_t *mutex)
{
    uint8_t locked;
    do
    {
        cli();
        locked = !*mutex;
        *mutex = 1;
        sei();
    }
    while (!locked);
}

uint8_t tiny_mutex_try_lock(tiny_mutex_t *mutex)
{
    uint8_t locked;
    cli();
    locked = !*mutex;
    *mutex = 1;
    sei();
    return locked;
}

void tiny_mutex_unlock(tiny_mutex_t *mutex)
{
    cli();
    *mutex = 0;
    sei();
}

void tiny_events_create(tiny_events_t *events)
{
    *events = 0;
}

void tiny_events_destroy(tiny_events_t *events)
{
}

uint8_t tiny_events_wait(tiny_events_t *events, uint8_t bits,
                         uint8_t clear, uint32_t timeout)
{
    uint8_t locked;
    uint32_t ts = tiny_millis();
    do
    {
        cli();
        locked = *events;
        if ( clear && (locked & bits) ) *events &= ~bits;
        sei();
        if (!(locked & bits) && (uint32_t)(tiny_millis() - ts) >= timeout)
        {
            locked = 0;
            break;
        }
    }
    while (!(locked & bits));
    return locked;
}

void tiny_events_set(tiny_events_t *events, uint8_t bits)
{
    cli();
    *events |= bits;
    sei();
}

void tiny_events_clear(tiny_events_t *events, uint8_t bits)
{
    cli();
    *events &= ~bits;
    sei();
}

void tiny_sleep(uint32_t ms)
{
    if (!ms) return;
#if defined(ARDUINO)
    return delay( ms );
#else
    while (ms--)
    {
        _delay_ms(1);
    }
#endif
}

uint32_t tiny_millis()
{
#if defined(ARDUINO)
    return millis();
#else
    return 0;
#endif
}

#endif
