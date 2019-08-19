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

#if defined(__TARGET_CPU_CORTEX_M0)

inline static int _iDisGetPrimask(void)
{
    int result;
    __asm__ __volatile__ ("MRS %0, primask" : "=r" (result) );
    __asm__ __volatile__ ("cpsid i" : : : "memory");
    return result;
}

inline static int _iSetPrimask(int priMask)
{
    __asm__ __volatile__ ("MSR primask, %0" : : "r" (priMask) : "memory");
    return 0;
}

#define ATOMIC_BLOCK \
     for(int mask = _iDisGetPrimask(), flag = 1;\
         flag;\
         flag = _iSetPrimask(mask))

//#define ATOMIC_OP(asm_op, a, v) do {
//        uint32_t reg0;
//        __asm__ __volatile__("cpsid i\n"
//                             "ldr %0, [%1]\n"
//                             #asm_op" %0, %0, %2\n"
//                             "str %0, [%1]\n"
//                             "cpsie i\n"
//                             : "=&b" (reg0)
//                             : "b" (a), "r" (v) : "cc" );
//       } while (0)

#else

#define ATOMIC_BLOCK

#endif

#include "../hal_single_core.inl"

void tiny_sleep(uint32_t ms)
{
    delay(ms);
}

uint32_t tiny_millis()
{
    return millis();
}

#endif

