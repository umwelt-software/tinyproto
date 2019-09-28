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

#include "proto/hal/tiny_types.h"
#include "tiny_fd.h"
#include "tiny_fd_int.h"

#include <string.h>

#ifndef TINY_FD_DEBUG
#define TINY_FD_DEBUG 0
#endif

#if TINY_FD_DEBUG
#include <stdio.h>
#define LOG(...)  fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

#define HDLC_I_FRAME_BITS  0x00
#define HDLC_I_FRAME_MASK  0x01

#define HDLC_S_FRAME_BITS  0x01
#define HDLC_S_FRAME_MASK  0x03
#define HDLC_S_FRAME_TYPE_REJ  0x04
#define HDLC_S_FRAME_TYPE_RR  0x00
#define HDLC_S_FRAME_TYPE_MASK 0x0C

enum
{
    FD_EVENT_TX_SENDING = 0x01,
    FD_EVENT_TX_DATA = 0x02,
    FD_EVENT_TX_QUEUE_FREE = 0x04,
};

static int write_func_cb(void *user_data, const void *data, int len);
static int on_frame_read(void *user_data, void *data, int len);
static int on_frame_sent(void *user_data, const void *data, int len);

///////////////////////////////////////////////////////////////////////////////

static void __request_control_frame(tiny_fd_handle_t handle, uint8_t type, uint8_t seq_num)
{
    handle->frames.control_cmd = HDLC_S_FRAME_BITS | type |
                                 ( seq_num << 5 );
    tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
}

///////////////////////////////////////////////////////////////////////////////

static int check_received_frame(tiny_fd_handle_t handle, uint8_t ns)
{
    int result = TINY_SUCCESS;
    tiny_mutex_lock( &handle->frames.mutex );
    if (ns == handle->frames.next_nr)
    {
        // this is what, we've been waiting for
        //LOG("[%p] Confirming received frame <= %d\n", handle, ns);
        handle->frames.next_nr = (handle->frames.next_nr + 1) & handle->frames.seq_bits;
        if ( handle->frames.next_ns == handle->frames.last_ns )
        {
            // nothing to send, send control frame
            __request_control_frame( handle, HDLC_S_FRAME_TYPE_RR, handle->frames.next_nr );
        }
    }
    else
    {
        // definitely we need to send reject. We want to see next_nr frame
        //LOG("[%p] Reject frame %d, need %d\n", handle, ns, handle->frames.next_nr);
        __request_control_frame( handle, HDLC_S_FRAME_TYPE_REJ, handle->frames.next_nr );
        result = TINY_ERR_FAILED;
    }
    tiny_mutex_unlock( &handle->frames.mutex );
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static void confirm_sent_frames(tiny_fd_handle_t handle, uint8_t nr)
{
    tiny_mutex_lock( &handle->frames.mutex );
    // all frames below nr are received
    while ( nr != handle->frames.confirm_ns )
    {
        //LOG("[%p] Confirming sent frames %d\n", handle, handle->frames.confirm_ns);
        handle->frames.confirm_ns = (handle->frames.confirm_ns + 1) & handle->frames.seq_bits;
        tiny_events_set( &handle->frames.events, FD_EVENT_TX_QUEUE_FREE );
    }
    // LOG("[%p] N(S)=%d, N(R)=%d\n", handle, handle->frames.confirm_ns, handle->frames.next_nr);
    tiny_mutex_unlock( &handle->frames.mutex );
}

///////////////////////////////////////////////////////////////////////////////

static void resend_all_unconfirmed_frames( tiny_fd_handle_t handle, uint8_t nr )
{
    tiny_mutex_lock( &handle->frames.mutex );
    handle->frames.next_ns = nr;
    tiny_mutex_unlock( &handle->frames.mutex );
    confirm_sent_frames( handle, nr );
}

///////////////////////////////////////////////////////////////////////////////

static int on_frame_read(void *user_data, void *data, int len)
{
    tiny_fd_handle_t handle = (tiny_fd_handle_t)user_data;
    //printf("[%p] Incoming frame of size %i\n", handle, len);
    if ( len < 2 )
    {
        LOG("FD: received too small frame\n");
        return TINY_ERR_FAILED;
    }
    uint8_t control = ((uint8_t *)data)[1];
    if ( (control & HDLC_I_FRAME_MASK) == HDLC_I_FRAME_BITS )
    {
        uint8_t nr = control >> 5;
        uint8_t ns = (control >> 1) & 0x07;
        int result = check_received_frame( handle, ns );
        confirm_sent_frames( handle, nr );
        // Provide data to user only if we expect this frame
        if ( result == TINY_SUCCESS )
        {
            /* inform user application about new frame received */
            if (handle->on_frame_cb)
            {
                handle->on_frame_cb( handle->user_data, 0, data, len - 2 );
            }
        }
    }
    else if ( (control & HDLC_S_FRAME_MASK) == HDLC_S_FRAME_BITS )
    {
        uint8_t nr = control >> 5;
        if ( (control & HDLC_S_FRAME_TYPE_MASK) == HDLC_S_FRAME_TYPE_REJ )
        {
            resend_all_unconfirmed_frames( handle, nr );
        }
        else if ( (control & HDLC_S_FRAME_TYPE_MASK) == HDLC_S_FRAME_TYPE_RR )
        {
            confirm_sent_frames( handle, nr );
        }
    }
    return len;
}

static int on_frame_sent(void *user_data, const void *data, int len)
{
    tiny_fd_handle_t handle = (tiny_fd_handle_t)user_data;
    uint8_t control = *(uint8_t *)data;
    if ( (control & HDLC_S_FRAME_MASK) == HDLC_I_FRAME_BITS )
    {
        tiny_events_set( &handle->frames.events, FD_EVENT_TX_QUEUE_FREE );
        // nothing to do for now
    }
    else if ( (control & HDLC_S_FRAME_MASK) == HDLC_S_FRAME_BITS )
    {
    }
    tiny_events_clear( &handle->frames.events, FD_EVENT_TX_SENDING );
    return len;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_init(tiny_fd_handle_t      * handle,
                 STinyFdInit      * init)
{
    if ( (0 == init->on_frame_cb) ||
         (0 == init->buffer) ||
         (0 == init->buffer_size) )
    {
        return TINY_ERR_FAILED;
    }
    if ( init->buffer_size < sizeof(tiny_fd_data_t) + sizeof(tiny_i_frame_info_t) * init->window_frames  )
    {
        LOG("DDD %i < %li, hdlc %li\n", init->buffer_size,
            sizeof(tiny_fd_data_t) + sizeof(tiny_i_frame_info_t) * init->window_frames,
            sizeof(hdlc_struct_t) );
        return TINY_ERR_INVALID_DATA;
    }
    memset(init->buffer, 0, init->buffer_size);

    tiny_fd_data_t *protocol = init->buffer;
    tiny_i_frame_info_t **i_frames = (tiny_i_frame_info_t **)((uint8_t *)protocol + sizeof(tiny_fd_data_t));
    uint8_t *tx_buffer = (uint8_t *)i_frames + sizeof(tiny_i_frame_info_t *) * init->window_frames;

    *handle = protocol;
    protocol->frames.i_frames = i_frames;
    protocol->frames.tx_buffer = tx_buffer;
    protocol->frames.seq_bits = init->window_frames - 1;
    protocol->frames.mtu = ((uint8_t *)init->buffer + init->buffer_size - tx_buffer) / (init->window_frames + 1);
    protocol->frames.rx_buffer = tx_buffer + protocol->frames.mtu * init->window_frames;
    for (int i=0; i < init->window_frames; i++)
        protocol->frames.i_frames[i] = (tiny_i_frame_info_t *)(i * protocol->frames.mtu + protocol->frames.tx_buffer);
    tiny_mutex_create( &protocol->frames.mutex );
    tiny_events_create( &protocol->frames.events );
    tiny_events_set( &protocol->frames.events, FD_EVENT_TX_QUEUE_FREE );

    protocol->_hdlc.send_tx = write_func_cb;
    protocol->_hdlc.on_frame_read = on_frame_read;
    protocol->_hdlc.on_frame_sent = on_frame_sent;
    protocol->_hdlc.user_data = *handle;
    protocol->_hdlc.rx_buf = protocol->frames.rx_buffer;
    protocol->_hdlc.rx_buf_size = protocol->frames.mtu;
    protocol->_hdlc.crc_type = init->crc_type;

    hdlc_init( &protocol->_hdlc );

    protocol->user_data = init->pdata;
    protocol->read_func = init->read_func;
    protocol->write_func = init->write_func;
    protocol->on_frame_cb = init->on_frame_cb;
    protocol->on_sent_cb = init->on_sent_cb;
    protocol->multithread_mode = init->multithread_mode;
    protocol->timeout = init->timeout ? init->timeout : 1000;
    return TINY_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_fd_close(tiny_fd_handle_t  handle)
{
    hdlc_close( &handle->_hdlc );
    tiny_events_destroy( &handle->frames.events );
    tiny_mutex_destroy( &handle->frames.mutex );
}

///////////////////////////////////////////////////////////////////////////////

static int write_func_cb(void *user_data, const void *data, int len)
{
    tiny_fd_handle_t handle = (tiny_fd_handle_t)user_data;
    return handle->write_func( handle->user_data, data, len );
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_run_rx(tiny_fd_handle_t handle, uint16_t timeout)
{
    return hdlc_run_rx_until_read( &handle->_hdlc, handle->read_func, handle->user_data, timeout );
}

///////////////////////////////////////////////////////////////////////////////

static uint8_t *tiny_fd_get_next_frame_to_send( tiny_fd_handle_t handle, int *len )
{
    uint8_t *data = NULL;
    // Tx data available
    tiny_mutex_lock( &handle->frames.mutex );
    if ( handle->frames.control_cmd != 0 )
    {
        *len = sizeof(tiny_s_frame_info_t);
        data = (uint8_t *)&handle->frames.s_frame.header;
        handle->frames.s_frame.header.address = 0xFF;
        handle->frames.s_frame.header.control = handle->frames.control_cmd;
        #ifdef TINY_FD_DEBUG
        LOG("[%p] Sending S-Frame N(R)=%02X, type=%s\n",
            handle, (handle->frames.s_frame.header.control >> 5),
            ((handle->frames.control_cmd >> 2) & 0x03) == 0x00 ? "RR" : "REJ");
        #endif
        handle->frames.control_cmd = 0;
    }
    else if ( handle->frames.next_ns != handle->frames.last_ns )
    {
        uint8_t i = handle->frames.next_ns;
        data = (uint8_t *)&handle->frames.i_frames[i]->header;
        *len = handle->frames.i_frames[i]->len + sizeof(tiny_frame_header_t);
        handle->frames.i_frames[i]->header.address = 0xFF;
        handle->frames.i_frames[i]->header.control = (handle->frames.next_ns << 1) |
                                                     (handle->frames.next_nr << 5);
        LOG("[%p] Sending I-Frame N(R)=%02X,N(S)=%02X\n", handle, handle->frames.next_nr, handle->frames.next_ns);
        handle->frames.next_ns = (i + 1) & handle->frames.seq_bits;
    }
    tiny_mutex_unlock( &handle->frames.mutex );
    return data;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_run_tx( tiny_fd_handle_t handle, uint16_t timeout )
{
    int result = TINY_ERR_TIMEOUT;
    if ( tiny_events_wait( &handle->frames.events, FD_EVENT_TX_SENDING, EVENT_BITS_LEAVE, 0 ) )
    {
        result = hdlc_run_tx( &handle->_hdlc );
    }
    else if ( tiny_events_wait( &handle->frames.events, FD_EVENT_TX_DATA, EVENT_BITS_CLEAR, timeout ) )
    {
        int len = 0;
        uint8_t *data = tiny_fd_get_next_frame_to_send( handle, &len );
        if ( data != NULL )
        {
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_SENDING );
            result = hdlc_send( &handle->_hdlc, data, len, timeout );
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_send( tiny_fd_handle_t handle, const void *data, int len, uint16_t timeout)
{
    int result;
    if ( len > handle->frames.mtu )
    {
        result = TINY_ERR_INVALID_DATA;
    }
    else if ( tiny_events_wait( &handle->frames.events, FD_EVENT_TX_QUEUE_FREE, EVENT_BITS_CLEAR, timeout ) )
    {
        tiny_mutex_lock( &handle->frames.mutex );
        handle->frames.i_frames[handle->frames.last_ns]->len = len;
        memcpy( &handle->frames.i_frames[handle->frames.last_ns]->user_payload, data, len );
        handle->frames.last_ns = (handle->frames.last_ns + 1) & handle->frames.seq_bits;
        tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
        if ( ((handle->frames.last_ns + 1) & handle->frames.seq_bits) != handle->frames.confirm_ns )
        {
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_QUEUE_FREE );
        }
        tiny_mutex_unlock( &handle->frames.mutex );
        result = TINY_SUCCESS;
    }
    else
    {
        result = TINY_ERR_TIMEOUT;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
