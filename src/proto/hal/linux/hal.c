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

#if defined(__linux__)

#include "proto/hal/tiny_types.h"

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

void tiny_mutex_create(tiny_mutex_t *mutex)
{
    pthread_mutex_init(mutex, NULL);
}

void tiny_mutex_destroy(tiny_mutex_t *mutex)
{
    pthread_mutex_destroy(mutex);
}

void tiny_mutex_lock(tiny_mutex_t *mutex)
{
    pthread_mutex_lock( mutex );
}

uint8_t tiny_mutex_try_lock(tiny_mutex_t *mutex)
{
    return pthread_mutex_trylock(mutex) == 0;
}

void tiny_mutex_unlock(tiny_mutex_t *mutex)
{
    pthread_mutex_unlock( mutex );
}

void tiny_events_create(tiny_events_t *events)
{
    events->bits = 0;
    pthread_mutex_init(&events->mutex, NULL);
    pthread_cond_init(&events->cond, NULL);
}

void tiny_events_destroy(tiny_events_t *events)
{
    pthread_cond_destroy(&events->cond);
    pthread_mutex_destroy(&events->mutex);
}

uint8_t tiny_events_wait_and_clear(tiny_events_t *events, uint8_t bits)
{
    uint8_t locked;
    pthread_mutex_lock( &events->mutex );
    while ( (events->bits & bits) != 0 )
    {
        pthread_cond_wait(&events->cond, &events->mutex);
    }
    locked = events->bits;
    events->bits &= ~bits;
    pthread_mutex_unlock( &events->mutex );
    return locked;
}

uint8_t tiny_events_wait(tiny_events_t *events, uint8_t bits)
{
    uint8_t locked;
    pthread_mutex_lock( &events->mutex );
    while ( (events->bits & bits) != 0 )
    {
        pthread_cond_wait(&events->cond, &events->mutex);
    }
    locked = events->bits;
    pthread_mutex_unlock( &events->mutex );
    return locked;
}

void tiny_events_set(tiny_events_t *events, uint8_t bits)
{
    pthread_mutex_lock( &events->mutex );
    events->bits |= bits;
    pthread_cond_broadcast(&events->cond);
    pthread_mutex_unlock( &events->mutex );
}

void tiny_events_clear(tiny_events_t *events, uint8_t bits)
{
    pthread_mutex_lock( &events->mutex );
    events->bits &= ~bits;
    pthread_mutex_unlock( &events->mutex );
}

void tiny_sleep( uint32_t millis )
{
    usleep( millis * 1000 );
}

uint32_t tiny_millis()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + ts.tv_nsec / 1000000;
}

#endif
