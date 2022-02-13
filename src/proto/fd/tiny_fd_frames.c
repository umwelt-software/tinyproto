/*
    Copyright 2021-2022 (,2022 (C) Alexey Dynda

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

#include "tiny_fd_frames_int.h"
#include "hal/tiny_debug.h"

#include <string.h>

#ifndef TINY_FD_DEBUG
#define TINY_FD_DEBUG 0
#endif

#if TINY_FD_DEBUG
#define LOG(...) TINY_LOG(__VA_ARGS__)
#else
#define LOG(...)
#endif

int tiny_fd_queue_init(tiny_fd_queue_t *queue, uint8_t *buffer,
                       int max_size, int max_frames, int mtu)
{
    uint8_t *ptr = buffer;
    queue->frames = (tiny_fd_frame_info_t **)(ptr);
    ptr += sizeof(tiny_fd_frame_info_t *) * max_frames;
    /* At this point we still have correct alignment in the buffer if init->window_frames is even.
     * pointer is 4 bytes on ARM 32-bit, and if window_frames is even, that gives us multiply of 8.
     */
    queue->size = max_frames;
    /* Lets allocate memory for TX frames, we have <window_frames> TX frames */
    for ( int i = 0; i < queue->size; i++ )
    {
        queue->frames[i] = (tiny_fd_frame_info_t *)ptr;
        /* mtu must be correctly aligned also, so the developer must use only mtu multiple of 8 on 32-bit ARM systems */
        ptr += mtu + sizeof(tiny_fd_frame_info_t) - sizeof(((tiny_fd_frame_info_t *)0)->payload);
    }
    if ( ptr > buffer + max_size )
    {
        LOG(TINY_LOG_CRIT, "Out of provided memory: provided %i bytes, used %i bytes\n", max_size, (int)(ptr - buffer));
        return TINY_ERR_INVALID_DATA;
    }
    queue->mtu = mtu;
    tiny_fd_queue_reset( queue );
    return (int)(ptr - buffer);
}

void tiny_fd_queue_reset(tiny_fd_queue_t *queue)
{
    for (int i=0; i < queue->size; i++)
    {
        queue->frames[i]->type = TINY_FD_QUEUE_FREE;
    }
    queue->lookup_index = 0;
}

void tiny_fd_queue_reset_for(tiny_fd_queue_t *queue, uint8_t address)
{
    for (int i=0; i < queue->size; i++)
    {
        if ( ( queue->frames[i]->header.address & 0xFC ) == (address & 0xFC) )
        {
            queue->frames[i]->type = TINY_FD_QUEUE_FREE;
        }
    }
}

tiny_fd_frame_info_t *tiny_fd_queue_allocate(tiny_fd_queue_t *queue, uint8_t type, const uint8_t *data, int len)
{
    tiny_fd_frame_info_t *ptr = len <= queue->mtu ?  tiny_fd_queue_get_next(queue, TINY_FD_QUEUE_FREE, 0, 0) : NULL;
    if ( ptr != NULL )
    {
        memcpy( &ptr->payload[0], data, len );
        ptr->len = len;
        ptr->type = type;
    }
    return ptr;
}

tiny_fd_frame_info_t *tiny_fd_queue_get_next(tiny_fd_queue_t *queue, uint8_t type, uint8_t address, uint8_t arg)
{
    tiny_fd_frame_info_t *ptr = NULL;
    int index = queue->lookup_index;
    for (int i=0; i < queue->size; i++)
    {
        // fprintf(stderr, "REC: type %02X address %02X, looking for type %02X addr %02X\n", queue->frames[index]->type, queue->frames[index]->header.address, type, address);
        if ( queue->frames[index]->type & type )
        {
            if ( queue->frames[index]->type == TINY_FD_QUEUE_FREE )
            {
                ptr = queue->frames[index];
                break;
            }
            // Check address for all frames
            if ( (address & 0xFC) == (queue->frames[index]->header.address & 0xFC) )
            {
                // fprintf(stderr, "REC FOUND: type %02X address %02X, looking for type %02X addr %02X\n", queue->frames[index]->type, queue->frames[index]->header.address, type, address);
                if ( queue->frames[index]->type != TINY_FD_QUEUE_I_FRAME )
                {
                    ptr = queue->frames[index];
                    break;
                }
                // Check for I-frame for the frame number
                if ( ( ( queue->frames[index]->header.control >> 1 ) & 0x07 )  == arg )
                {
                    ptr = queue->frames[index];
                    break;
                }
            }
        }
        index++;
        if ( index >= queue->size )
        {
            index -= queue->size;
        }
    }
    return ptr;
}

void tiny_fd_queue_free(tiny_fd_queue_t *queue, tiny_fd_frame_info_t *frame)
{
    tiny_fd_queue_free_by_header(queue, &frame->header);
}

void tiny_fd_queue_free_by_header(tiny_fd_queue_t *queue, const void *header)
{
    for (int i=0; i < queue->size; i++)
    {
        if ( &queue->frames[i]->header == header )
        {
            queue->frames[i]->type = TINY_FD_QUEUE_FREE;
            queue->lookup_index = i + 1;
            if ( queue->lookup_index >= queue->size )
            {
                queue->lookup_index -= queue->size;
            }
            break;
        }
    }
}

int tiny_fd_queue_get_mtu(tiny_fd_queue_t *queue)
{
    return queue->mtu;
}

bool tiny_fd_queue_has_free_slots(tiny_fd_queue_t *queue)
{
    return tiny_fd_queue_get_next(queue, TINY_FD_QUEUE_FREE, 0, 0) != NULL;
}
