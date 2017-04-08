/*
    Copyright 2017 (C) Alexey Dynda

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

#include "tiny_defines.h"
#include "tiny_rq_pool.h"
#include "tiny_hd.h"

///////////////////////////////////////////////////////////////////////////////

static int tiny_wait_request_update(STinyHdData *handle, tiny_request * request, uint16_t timeout)
{
    uint16_t ticks = PLATFORM_TICKS();
    do
    {
        if (handle->multithread_mode)
        {
            /* TODO: Wait in blocking mode */
        }
        else
        {
            int result = tiny_hd_run( handle );
            if ( result < 0 )
            {
                return result;
            }
        }
        if ( request->state != TINY_HD_REQUEST_INIT )
        {
            break;
        }
        TASK_YIELD();
    } while ( PLATFORM_TICKS() - ticks < timeout );
    switch ( request->state )
    {
        case TINY_HD_REQUEST_INIT: return TINY_ERR_TIMEOUT;
        case TINY_HD_REQUEST_SUCCESSFUL: return TINY_SUCCESS;
        default:
            break;
    }
    return TINY_ERR_FAILED;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_send_wait_ack(STinyHdData *handle, void *buf, uint16_t len)
{
    int result = -1;
    tiny_request request;
    uint8_t retry;
    tiny_put_request(&request);
    request.uid |= 0x4000;
    for(retry=0; retry<3; retry++)
    {
        result = tiny_send( &handle->handle, &request.uid, buf, len, TINY_FLAG_WAIT_FOREVER);
        if (result <= 0)
        {
            tiny_remove_request(&request);
            return result;
        }
        result = tiny_wait_request_update(handle, &request, handle->timeout);
        if (result != TINY_ERR_TIMEOUT)
        {
            break;
        }
    }
    tiny_remove_request(&request);
    if ( TINY_HD_REQUEST_DECLINED == request.state)
    {
        return TINY_ERR_FAILED;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static void tiny_hd_on_frame(void *handle, uint16_t uid, uint8_t *buf, int len)
{
    /** response */
    if ( uid & 0x8000 )
    {
        /* this is response */
        tiny_commit_request( uid & ~(0x8000) );
        return;
    }
    if ( uid & 0x4000 )
    {
        uint8_t flag = 0;
        uid |= 0x8000;
        int result = tiny_send( handle, &uid, &flag, sizeof(flag), TINY_FLAG_WAIT_FOREVER );
        if ( result <= 0)
        {
            return;
        }
    }
    if (0 == ((STinyHdData *)handle)->on_frame_cb)
    {
        return;
    }
    uid &= 0x0FFF;
    /* inform user application about new frame received */
    ((STinyHdData *)handle)->on_frame_cb( handle, uid, buf, len );
    return;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_hd_init(STinyHdData      * handle,
                 STinyHdInit      * init)
{
    if ( (0 == init->on_frame_cb) ||
         (0 == init->inbuf) ||
         (0 == init->inbuf_size) )
    {
        return TINY_ERR_FAILED;
    }
    tiny_init( &handle->handle, init->write_func, init->read_func, init->pdata );
    /* Tiny HD works only with UID support */
    tiny_enable_uid(&handle->handle, 1);
    handle->on_frame_cb = init->on_frame_cb;
    handle->inbuf = init->inbuf;
    handle->inbuf_size = init->inbuf_size;
    tiny_set_callbacks(&handle->handle, tiny_hd_on_frame, 0);
    handle->multithread_mode = init->multithread_mode;
    handle->timeout = init->timeout ? init->timeout : 1000;
    return TINY_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_hd_close(STinyHdData  * handle)
{
    tiny_decline_requests();
    tiny_close( &handle->handle );
}

///////////////////////////////////////////////////////////////////////////////

int tiny_hd_run(STinyHdData *handle)
{
    return tiny_read( &handle->handle, &handle->uid, handle->inbuf, handle->inbuf_size, TINY_FLAG_NO_WAIT );
}

///////////////////////////////////////////////////////////////////////////////

