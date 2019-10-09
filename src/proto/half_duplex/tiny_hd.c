/*
    Copyright 2017-2019 (C) Alexey Dynda

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

#include "proto/hal/tiny_types.h"
#include "tiny_rq_pool.h"
#include "tiny_hd.h"
#include "proto/hal/tiny_debug.h"

#include <string.h>

#ifndef TINY_HD_DEBUG
#define TINY_HD_DEBUG 0
#endif

#if TINY_HD_DEBUG
#define LOG(...)  TINY_LOG(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define ACK_FRAME_FLAG 0x8000
#define DATA_FRAME_FLAG 0x4000

static int write_func_cb(void *user_data, const void *data, int len);
static int on_frame_read(void *user_data, void *data, int len);
static int on_frame_sent(void *user_data, const void *data, int len);

///////////////////////////////////////////////////////////////////////////////

static int tiny_wait_request_update(STinyHdData *handle, tiny_request * request, uint16_t timeout)
{
    uint16_t ticks = tiny_millis();
    do
    {
        if (handle->multithread_mode)
        {
            /* TODO: Wait in blocking mode */
            tiny_sleep(1);
        }
        else
        {
            int result = tiny_hd_run( handle );
            if ( result < 0 && result != TINY_ERR_TIMEOUT )
            {
                return result;
            }
        }
        if ( request->state != TINY_HD_REQUEST_INIT )
        {
            break;
        }
    } while ( (uint16_t)(tiny_millis() - ticks) < timeout );
    int result = TINY_ERR_FAILED;
    switch ( request->state )
    {
        case TINY_HD_REQUEST_INIT: result = TINY_ERR_TIMEOUT; break;
        case TINY_HD_REQUEST_SUCCESSFUL: result =  TINY_SUCCESS; break;
        default: break;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_send_wait_ack(STinyHdData *handle, void *buf, uint16_t len)
{
    LOG( TINY_LOG_DEB, "[%p] tiny_send_wait_ack %s mode\n", handle, handle->multithread_mode ? "multithread" : "singlethread" );
    int result = TINY_ERR_FAILED;
    tiny_request request;
    uint8_t retry;
    if ( len > handle->_hdlc.rx_buf_size ) return TINY_ERR_FAILED;
    tiny_put_request(&request);
    request.uid |= DATA_FRAME_FLAG;
    for(retry=0; retry<3; retry++)
    {
        // WARNING!!! reuse of rx buffer
        uint8_t *data = (uint8_t *)handle->_hdlc.rx_buf + handle->_hdlc.rx_buf_size / 2;
        data[0] = (request.uid >> 8) & 0xFF;
        data[1] = request.uid & 0xFF;
        memcpy( data + sizeof(uint16_t), buf, len);
        LOG( TINY_LOG_INFO, "[%p] >>>> %04X (DATA)\n", handle, request.uid);
        result = hdlc_send( &handle->_hdlc , data, len + sizeof(uint16_t), handle->timeout);
        if (result < 0)
        {
            LOG( TINY_LOG_ERR, "[%p] >>>> %04X (DATA FAILED)\n", handle, request.uid);
            tiny_remove_request(&request);
            return result;
        }
        result = tiny_wait_request_update(handle, &request, handle->timeout);
        // exit on error or success, continue on timeout
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
    if ( result == TINY_SUCCESS )
    {
        if (handle->on_sent_cb)
        {
            handle->on_sent_cb(handle->user_data, request.uid & 0x0FFF, buf, len);
        }
        result = len;
    }
    if (result == TINY_ERR_TIMEOUT)
    {
        LOG( TINY_LOG_WRN, "[%p] >>>> %04X (DATA TIMEOUT)\n", handle, request.uid);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static int on_frame_read(void *user_data, void *data, int len)
{
    uint16_t uid = (((uint16_t)((uint8_t *)data)[1]) << 0) |
                   (((uint16_t)((uint8_t *)data)[0]) << 8);
    STinyHdData * handle = (STinyHdData *)user_data;
    /** response */
    if ( uid & ACK_FRAME_FLAG )
    {
        LOG( TINY_LOG_INFO, "[%p] <<<< %04X (I,ACK)\n", user_data, uid);
        /* this is response. Mark frame as successfully delivered and exit */
        tiny_commit_request( uid & ~(ACK_FRAME_FLAG) );
        return len;
    }
    if ( uid & DATA_FRAME_FLAG )
    {
        LOG( TINY_LOG_INFO, "[%p] <<<< %04X (DATA)\n", user_data, uid);
        uid |= ACK_FRAME_FLAG;
        uint8_t *buf = (uint8_t *)handle->_hdlc.rx_buf + handle->_hdlc.rx_buf_size;
        buf[0] = (uid >> 8) & 0xFF;
        buf[1] = uid & 0xFF;
        LOG( TINY_LOG_INFO, "[%p] >>>> %04X (O,ACK)\n", user_data, uid);
        int result = hdlc_send( &handle->_hdlc, buf, sizeof(uid), handle->timeout );
        if ( result < 0)
        {
            LOG( TINY_LOG_ERR, "[%p] >>>> %04X (O,ACK FAILED)\n", user_data, uid);
            return result; // return error if failed to send ACK
        }
    }
    if (0 == handle->on_frame_cb)
    {
        return len;
    }
    uint8_t * buf = (uint8_t *)data + sizeof(uid);
    uid &= 0x0FFF;
    /* inform user application about new frame received */
    handle->on_frame_cb( handle->user_data, uid, buf, len - 2 );
    return len;
}

static int on_frame_sent(void *user_data, const void *data, int len)
{
    ((STinyHdData *)user_data)->_sent++;
    return len;
}

///////////////////////////////////////////////////////////////////////////////
void tiny_list_init(void);

int tiny_hd_init(STinyHdData      * handle,
                 STinyHdInit      * init)
{
    if ( (0 == init->on_frame_cb) ||
         (0 == init->inbuf) ||
         (0 == init->inbuf_size) )
    {
        return TINY_ERR_FAILED;
    }
    tiny_list_init();
    handle->_hdlc.send_tx = write_func_cb;
    handle->_hdlc.on_frame_read = on_frame_read;
    handle->_hdlc.on_frame_sent = on_frame_sent;
    handle->_hdlc.user_data = handle;
    handle->_hdlc.rx_buf = init->inbuf;
    handle->_hdlc.rx_buf_size = init->inbuf_size - sizeof(uint16_t);
    handle->_hdlc.crc_type = init->crc_type;
    handle->_hdlc.multithread_mode = init->multithread_mode;

    hdlc_init( &handle->_hdlc );

    handle->user_data = init->pdata;
    handle->read_func = init->read_func;
    handle->write_func = init->write_func;
    handle->on_frame_cb = init->on_frame_cb;
    handle->on_sent_cb = init->on_sent_cb;
    handle->multithread_mode = init->multithread_mode;
    handle->timeout = init->timeout ? init->timeout : 1000;
    return TINY_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_hd_close(STinyHdData  * handle)
{
    tiny_decline_requests();
    hdlc_close( &handle->_hdlc );
}

///////////////////////////////////////////////////////////////////////////////

static int write_func_cb(void *user_data, const void *data, int len)
{
    STinyHdData * handle = (STinyHdData *)user_data;
    return handle->write_func( handle->user_data, data, len );
}

///////////////////////////////////////////////////////////////////////////////

int tiny_hd_run(STinyHdData *handle)
{
    return hdlc_run_rx_until_read( &handle->_hdlc, handle->read_func, handle->user_data, 100 );
}

///////////////////////////////////////////////////////////////////////////////

int tiny_hd_run_tx(STinyHdData * handle)
{
    return hdlc_run_tx( &handle->_hdlc );
}

///////////////////////////////////////////////////////////////////////////////

