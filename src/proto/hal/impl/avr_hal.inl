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

#if defined(ARDUINO)
#include <Arduino.h>
#endif
// TODO: <util/atomic.h>

#include "proto/hal/tiny_types.h"
#include <avr/interrupt.h>
#include <util/delay.h>

inline static int _iDisGetPrimask(void)
{
    cli();
    return 0;
}

inline static int _iSetPrimask(int priMask)
{
    sei();
    return 0;
}

#define ATOMIC_BLOCK \
     for(int mask = _iDisGetPrimask(), flag = 1;\
         flag;\
         flag = _iSetPrimask(mask))

#include "hal_single_core.inl"

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


