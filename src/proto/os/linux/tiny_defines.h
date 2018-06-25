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

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

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
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + ts.tv_nsec / 1000000;
}


#endif /* _TINY_DEFINES_H_ */


