/*
    Copyright 2019-2021 (C) Alexey Dynda

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

static void my_platform_mutex_create(tiny_mutex_t *mutex)
{
    /* Here tiny_mutex_t is uintptr_t type.
     * You can use it the way you need in your platform, i.e. to store pointer to platform mutex structure,
     * or to store the flag like in the example below  */
    *mutex = 0;
}

static void my_platform_mutex_destroy(tiny_mutex_t *mutex)
{
}

static void my_platform_mutex_lock(tiny_mutex_t *mutex)
{
    uint8_t locked;
    do
    {
        locked = tiny_mutex_try_lock( mutex );
        if ( locked )
        {
            break;
        }
        tiny_sleep(1);
    } while (!locked);
}

static uint8_t my_platform_mutex_try_lock(tiny_mutex_t *mutex)
{
    uint8_t locked = __sync_bool_compare_and_swap( mutex, 0, 1 );
    locked = !!locked;
    return locked;
}

static void my_platform_mutex_unlock(tiny_mutex_t *mutex)
{
     __sync_bool_compare_and_swap( mutex, 1, 0 );
}

static void my_platform_events_create(tiny_events_t *events)
{
    /* Here tiny_events_t is the structure with 2 fields: tiny_mutex_t, which is uintptr_t, and uint8_t bits field for event flags */
    tiny_mutex_create( &events->mutex );
    events->bits = 0;
}

static void my_platform_events_destroy(tiny_events_t *events)
{
    tiny_mutex_destroy( &events->mutex );
}

static uint8_t my_platform_events_wait(tiny_events_t *events, uint8_t bits,
                         uint8_t clear, uint32_t timeout)
{
    uint8_t locked = 0;
    uint32_t ts = tiny_millis();
    do
    {
        tiny_mutex_lock( &events->mutex );
        locked = events->bits;
        if ( clear && (locked & bits) ) events->bits &= ~bits;
        tiny_mutex_unlock( &events->mutex );
        if (!(locked & bits) && (uint32_t)(tiny_millis() - ts) >= timeout)
        {
            locked = 0;
            break;
        }
        tiny_sleep(1);
     }
    while (!(locked & bits));
    return locked;
}

static uint8_t my_platform_events_check_int(tiny_events_t *events, uint8_t bits, uint8_t clear)
{
    uint8_t locked;
    tiny_mutex_lock( &events->mutex );
    locked = events->bits;
    if ( clear && (locked & bits) ) events->bits &= ~bits;
    tiny_mutex_unlock( &events->mutex );
    return locked;
}

static void my_platform_events_set(tiny_events_t *events, uint8_t bits)
{
    tiny_mutex_lock( &events->mutex );
    events->bits |= bits;
    tiny_mutex_unlock( &events->mutex );
}

static void my_platform_events_clear(tiny_events_t *events, uint8_t bits)
{
    tiny_mutex_lock( &events->mutex );
    events->bits &= ~bits;
    tiny_mutex_unlock( &events->mutex );
}

static void my_platform_sleep(uint32_t ms)
{
    // No default support for sleep
    uint32_t start = tiny_millis();
    while ( (uint32_t)(tiny_millis() - start) < ms );
}

static uint32_t my_platform_millis()
{
    // No default support for milliseconds
    static uint32_t cnt = 0;
    return cnt++;
}

void my_platform_tiny_hal_init(tiny_platform_hal_t *hal)
{
    tiny_platform_hal_t hal =
    {
        .mutex_create = my_platform_mutex_create,
        .mutex_destroy = my_platform_mutex_destroy,
        .mutex_try_lock = my_platform_mutex_try_lock,
        .mutex_unlock = my_platform_mutex_unlock,
        .mutex_lock = my_platform_mutex_lock,

        .events_create = my_platform_events_create,
        .events_destroy = my_platform_events_destroy,
        .events_wait = my_platform_events_wait,
        .events_check_int = my_platform_events_check_int,
        .events_set = my_platform_events_set,
        .events_clear = my_platform_events_clear,

        .sleep = my_platform_sleep,
        .millis = my_platform_millis,
    };

    tiny_hal_init( &hal );
}
