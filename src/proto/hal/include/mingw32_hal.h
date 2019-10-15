/*
    Copyright 2016,2019 (C) Alexey Dynda

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

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>

#ifndef CONFIG_ENABLE_CHECKSUM
#   define CONFIG_ENABLE_CHECKSUM
#endif

#ifndef CONFIG_ENABLE_FCS16
#   define CONFIG_ENABLE_FCS16
#endif

#ifndef CONFIG_ENABLE_FCS32
#   define CONFIG_ENABLE_FCS32
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/**
 * Mutex type used by Tiny Protocol implementation.
 * The type declaration depends on platform.
 */
typedef pthread_mutex_t tiny_mutex_t;

/**
 * Event groups type used by Tiny Protocol implementation.
 * The type declaration depends on platform.
 */
typedef struct
{
    uint8_t bits;
    uint16_t waiters;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} tiny_events_t;

//#define TASK_YIELD()      Sleep(0)


/*static inline uint16_t PLATFORM_TICKS()
{
    return GetTickCount();
}
*/

#if 0

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <Windows.h>

#define PLATFORM_MUTEX    pthread_mutex_t

#define MUTEX_INIT(x)     pthread_mutex_init(&x, NULL)

#define MUTEX_LOCK(x)     pthread_mutex_lock(&x)

#define MUTEX_TRY_LOCK(x) pthread_mutex_trylock(&x)

#define MUTEX_UNLOCK(x)   pthread_mutex_unlock(&x)

#define MUTEX_DESTROY(x)  pthread_mutex_destroy(&x)

#define PLATFORM_COND     pthread_cond_t

#define COND_INIT(x)      pthread_cond_init(&x, NULL)

#define COND_DESTROY(x)   pthread_cond_destroy(&x)

#define COND_WAIT(cond, mutex)   pthread_cond_wait(&cond, &mutex)

#define COND_SIGNAL(x)    pthread_cond_signal(&x)

#define TASK_YIELD()      sleep(0)


static inline uint16_t PLATFORM_TICKS()
{
    return GetTickCount();
}

#endif

#endif
