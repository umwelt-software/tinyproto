/*
    Copyright 2016 (C) Alexey Dynda

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

#ifndef _TINY_DEFINES_H_
#define _TINY_DEFINES_H_

#include "tiny_config.h"

/* For fastest version of protocol assign all defines to zero.
 * In this case protocol supports no CRC field, and
 * all api functions become not thread-safe.
 */

#undef  PLATFORM_MUTEX

#define MUTEX_INIT(x)

#define MUTEX_LOCK(x)

#define MUTEX_TRY_LOCK(x)    0

#define MUTEX_UNLOCK(x)

#define MUTEX_DESTROY(x)

#undef  PLATFORM_COND

#define COND_INIT(x)

#define COND_DESTROY(x)

#define COND_WAIT(cond, mutex)

#define COND_SIGNAL(x)

#define TASK_YIELD()

#define PLATFORM_TICKS()    millis()

#ifdef __linux__
#include <stdint.h>
static inline uint32_t millis() { return 0; }
#endif

#endif /* _TINY_DEFINES_H_ */


