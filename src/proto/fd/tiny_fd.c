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
#include "proto/hal/tiny_debug.h"

#include <string.h>

#ifndef TINY_FD_DEBUG
#define TINY_FD_DEBUG 0
#endif

#if TINY_FD_DEBUG
#define LOG(...)  TINY_LOG(__VA_ARGS__)
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

#define HDLC_U_FRAME_BITS  0x03
#define HDLC_U_FRAME_MASK  0x03
#define HDLC_U_FRAME_TYPE_UA   0x60
#define HDLC_U_FRAME_TYPE_FRMR 0x84
#define HDLC_U_FRAME_TYPE_RSET 0x8C
#define HDLC_U_FRAME_TYPE_SABM 0x2C
#define HDLC_U_FRAME_TYPE_MASK 0xEC

#define HDLC_P_BIT         0x10
#define HDLC_F_BIT         0x10

enum
{
    FD_EVENT_TX_SENDING = 0x01,
    FD_EVENT_TX_DATA = 0x02,
    FD_EVENT_I_QUEUE_FREE = 0x04,
};

static int write_func_cb(void *user_data, const void *data, int len);
static int on_frame_read(void *user_data, void *data, int len);
static int on_frame_sent(void *user_data, const void *data, int len);

///////////////////////////////////////////////////////////////////////////////

static void __send_control_frame(tiny_fd_handle_t handle, uint8_t control)
{
    handle->frames.control_cmds <<= 8;
    handle->frames.control_cmds |= control;
    tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
}

static void __send_u_frame(tiny_fd_handle_t handle, uint8_t control)
{
    __send_control_frame( handle, control | HDLC_U_FRAME_BITS );
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
            // nothing to send, notify to send control frame
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
        }
        handle->frames.sent_reject = 0;
    }
    else
    {
        // definitely we need to send reject. We want to see next_nr frame
        if ( !handle->frames.sent_reject )
        {
            handle->frames.sent_nr |= 0x80;
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
        }
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
        tiny_events_set( &handle->frames.events, FD_EVENT_I_QUEUE_FREE );
    }
    // LOG("[%p] N(S)=%d, N(R)=%d\n", handle, handle->frames.confirm_ns, handle->frames.next_nr);
    tiny_mutex_unlock( &handle->frames.mutex );
}

///////////////////////////////////////////////////////////////////////////////

static void resend_all_unconfirmed_frames( tiny_fd_handle_t handle, uint8_t nr )
{
    tiny_mutex_lock( &handle->frames.mutex );
    handle->frames.next_ns = nr;
    tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
    tiny_mutex_unlock( &handle->frames.mutex );
}

///////////////////////////////////////////////////////////////////////////////

static int on_i_frame_read( tiny_fd_handle_t handle, void *data, int len )
{
    uint8_t control = ((uint8_t *)data)[1];
    uint8_t nr = control >> 5;
    uint8_t ns = (control >> 1) & 0x07;
    int result = TINY_ERR_FAILED;
    LOG(TINY_LOG_INFO, "[%p] Receiving I-Frame N(R)=%02X,N(S)=%02X\n", handle, nr, ns);
    if ( handle->state == TINY_FD_STATE_CONNECTED_ABM )
    {
        result = check_received_frame( handle, ns );
        confirm_sent_frames( handle, nr );
        // Provide data to user only if we expect this frame
        if ( result == TINY_SUCCESS )
        {
            /* inform user application about new frame received */
            if (handle->on_frame_cb)
            {
                handle->on_frame_cb( handle->user_data, 0, (uint8_t *)data + 2, len - 2 );
            }
        }
    }
    else
    {
        LOG(TINY_LOG_ERR, "[%p] ABM connection is not established\n", handle);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static int on_s_frame_read( tiny_fd_handle_t handle, void *data, int len )
{
    uint8_t control = ((uint8_t *)data)[1];
    uint8_t nr = control >> 5;
    int result = TINY_ERR_FAILED;
    LOG(TINY_LOG_INFO, "[%p] Receiving S-Frame N(R)=%02X, type=%s\n",
         handle, nr, ((control >> 2) & 0x03) == 0x00 ? "RR" : "REJ");
    if ( handle->state == TINY_FD_STATE_CONNECTED_ABM )
    {
        if ( (control & HDLC_S_FRAME_TYPE_MASK) == HDLC_S_FRAME_TYPE_REJ )
        {
            confirm_sent_frames( handle, nr );
            resend_all_unconfirmed_frames( handle, nr );
        }
        else if ( (control & HDLC_S_FRAME_TYPE_MASK) == HDLC_S_FRAME_TYPE_RR )
        {
            confirm_sent_frames( handle, nr );
        }
    }
    else
    {
        LOG(TINY_LOG_ERR, "[%p] ABM connection is not established\n", handle);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static int on_u_frame_read( tiny_fd_handle_t handle, void *data, int len )
{
    uint8_t control = ((uint8_t *)data)[1];
    uint8_t type = control & HDLC_U_FRAME_TYPE_MASK;
    int result = TINY_ERR_FAILED;
    if ( type == HDLC_U_FRAME_TYPE_SABM )
    {
        tiny_mutex_lock( &handle->frames.mutex );
        __send_u_frame( handle, HDLC_U_FRAME_TYPE_UA | HDLC_F_BIT );
        tiny_mutex_unlock( &handle->frames.mutex );
    }
    else if ( type == HDLC_U_FRAME_TYPE_RSET )
    {
        // resets N(R) = 0 in secondary, resets N(S) = 0 in primary
        // expected answer UA
    }
    else if ( type == HDLC_U_FRAME_TYPE_FRMR )
    {
        // response of secondary in case of protocol errors: invalid control field, invalid N(R),
        // information field too long or not expected in this frame
    }
    else if ( type == HDLC_U_FRAME_TYPE_UA )
    {
        // confirmation received
        handle->state = TINY_FD_STATE_CONNECTED_ABM;
        tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
    }
    else
    {
        LOG(TINY_LOG_WRN, "[%p] Unknown hdlc U-frame received\n", handle);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static int on_frame_read(void *user_data, void *data, int len)
{
    tiny_fd_handle_t handle = (tiny_fd_handle_t)user_data;
    //printf("[%p] Incoming frame of size %i\n", handle, len);
    if ( len < 2 )
    {
        LOG(TINY_LOG_WRN, "FD: received too small frame\n");
        return TINY_ERR_FAILED;
    }
    uint8_t control = ((uint8_t *)data)[1];
    if ( (control & HDLC_I_FRAME_MASK) == HDLC_I_FRAME_BITS )
    {
        on_i_frame_read( handle, data, len );
    }
    else if ( (control & HDLC_S_FRAME_MASK) == HDLC_S_FRAME_BITS )
    {
        on_s_frame_read( handle, data, len );
    }
    else if ( (control & HDLC_U_FRAME_MASK) == HDLC_U_FRAME_MASK )
    {
        on_u_frame_read( handle, data, len );
    }
    else
    {
        LOG(TINY_LOG_WRN, "[%p] Unknown hdlc frame received\n", handle);
    }
    return len;
}

static int on_frame_sent(void *user_data, const void *data, int len)
{
    tiny_fd_handle_t handle = (tiny_fd_handle_t)user_data;
    uint8_t control = *(uint8_t *)data;
    if ( (control & HDLC_S_FRAME_MASK) == HDLC_I_FRAME_BITS )
    {
        // check for more data to send
        tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
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
        LOG(TINY_LOG_CRIT, "DDD %i < %li, hdlc %li\n", init->buffer_size,
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
    tiny_events_set( &protocol->frames.events, FD_EVENT_I_QUEUE_FREE );

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
    protocol->state = TINY_FD_STATE_DISCONNECTED;

    // Request remote side for ABM
    __send_u_frame( *handle, HDLC_P_BIT | HDLC_U_FRAME_TYPE_SABM );
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
    uint16_t ts = tiny_millis();
    int result;
    do
    {
        uint8_t data;
        result = handle->read_func( handle->user_data, &data, sizeof(data) );
        if ( result > 0 )
        {
            hdlc_run_rx( &handle->_hdlc, &data, sizeof(data), &result );
        }
    } while ((uint16_t)(tiny_millis() - ts) < timeout);
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static uint8_t *tiny_fd_get_next_frame_to_send( tiny_fd_handle_t handle, int *len )
{
    uint8_t *data = NULL;
    // Tx data available
    tiny_mutex_lock( &handle->frames.mutex );
    if ( handle->frames.control_cmds != 0 )
    {
        *len = sizeof(tiny_s_frame_info_t);
        data = (uint8_t *)&handle->frames.s_frame.header;
        handle->frames.s_frame.header.address = 0xFF;
        do
        {
            handle->frames.s_frame.header.control = handle->frames.control_cmds >> 24;
            handle->frames.control_cmds <<= 8;
        } while (handle->frames.s_frame.header.control == 0);
        LOG(TINY_LOG_INFO, "[%p] Sending U-Frame type=%02X\n",
            handle, handle->frames.s_frame.header.control & HDLC_U_FRAME_TYPE_MASK);
    }
    else if ( handle->frames.sent_nr > 127 && handle->state == TINY_FD_STATE_CONNECTED_ABM && !handle->frames.sent_reject )
    {
        *len = sizeof(tiny_s_frame_info_t);
        data = (uint8_t *)&handle->frames.s_frame.header;
        handle->frames.s_frame.header.address = 0xFF;
        handle->frames.s_frame.header.control = HDLC_S_FRAME_BITS | HDLC_S_FRAME_TYPE_REJ | ( handle->frames.next_nr << 5 );
        handle->frames.sent_nr = handle->frames.next_nr;
        handle->frames.sent_reject = 1;
        LOG(TINY_LOG_INFO, "[%p] Sending S-Frame N(R)=%02X, type=%s\n",
            handle, (handle->frames.s_frame.header.control >> 5),
            ((handle->frames.s_frame.header.control >> 2) & 0x03) == 0x00 ? "RR" : "REJ");
    }
    else if ( handle->frames.next_ns != handle->frames.last_ns && handle->state == TINY_FD_STATE_CONNECTED_ABM )
    {
        uint8_t i = (handle->frames.next_ns + handle->frames.ns_offset) & handle->frames.seq_bits;
        data = (uint8_t *)&handle->frames.i_frames[i]->header;
        *len = handle->frames.i_frames[i]->len + sizeof(tiny_frame_header_t);
        handle->frames.i_frames[i]->header.address = 0xFF;
        handle->frames.i_frames[i]->header.control = (handle->frames.next_ns << 1) |
                                                     (handle->frames.next_nr << 5);
        handle->frames.sent_nr = handle->frames.next_nr;
        LOG(TINY_LOG_INFO, "[%p] Sending I-Frame N(R)=%02X,N(S)=%02X\n", handle, handle->frames.next_nr, handle->frames.next_ns);
        handle->frames.next_ns++;
        handle->frames.next_ns &= handle->frames.seq_bits;
    }
    else if ( handle->frames.sent_nr < 128 && handle->frames.sent_nr != handle->frames.next_nr && handle->state == TINY_FD_STATE_CONNECTED_ABM )
    {
        *len = sizeof(tiny_s_frame_info_t);
        data = (uint8_t *)&handle->frames.s_frame.header;
        handle->frames.s_frame.header.address = 0xFF;
        handle->frames.s_frame.header.control = HDLC_S_FRAME_BITS | HDLC_S_FRAME_TYPE_RR | ( handle->frames.next_nr << 5 );
        handle->frames.sent_nr = handle->frames.next_nr;
        LOG(TINY_LOG_INFO, "[%p] Sending S-Frame N(R)=%02X, type=%s\n",
            handle, (handle->frames.s_frame.header.control >> 5),
            ((handle->frames.s_frame.header.control >> 2) & 0x03) == 0x00 ? "RR" : "REJ");
    }
    tiny_mutex_unlock( &handle->frames.mutex );
    return data;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_run_tx( tiny_fd_handle_t handle, uint16_t timeout )
{
    int result = TINY_ERR_TIMEOUT;
    // Check if send on hdlc level operation is in progress and do some work
    if ( tiny_events_wait( &handle->frames.events, FD_EVENT_TX_SENDING, EVENT_BITS_LEAVE, 0 ) )
    {
        result = hdlc_run_tx( &handle->_hdlc );
    }
    // Since no send operation is in progress, check if we have something to send
    else if ( tiny_events_wait( &handle->frames.events, FD_EVENT_TX_DATA, EVENT_BITS_CLEAR, timeout ) )
    {
        int len = 0;
        uint8_t *data = tiny_fd_get_next_frame_to_send( handle, &len );
        if ( data != NULL )
        {
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_SENDING );
            result = hdlc_send( &handle->_hdlc, data, len, handle->timeout );
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_send( tiny_fd_handle_t handle, const void *data, int len, uint16_t timeout)
{
    int result;
    LOG( TINY_LOG_DEB, "[%p] PUT frame\n", handle );
    // Check frame size againts mtu
    if ( len > handle->frames.mtu )
    {
        LOG( TINY_LOG_ERR, "[%p] PUT frame error\n", handle );
        result = TINY_ERR_DATA_TOO_LARGE;
    }
    // Wait until there is room for new frame
    else if ( tiny_events_wait( &handle->frames.events, FD_EVENT_I_QUEUE_FREE, EVENT_BITS_CLEAR, timeout ) )
    {
        tiny_mutex_lock( &handle->frames.mutex );
        uint8_t new_last_ns = (handle->frames.last_ns + 1) & handle->frames.seq_bits;
        // Check if space is available
        if ( new_last_ns != handle->frames.confirm_ns )
        {
            uint8_t i = (handle->frames.last_ns + handle->frames.ns_offset) & handle->frames.seq_bits;
            handle->frames.i_frames[i]->len = len;
            memcpy( &handle->frames.i_frames[i]->user_payload, data, len );
            handle->frames.last_ns = new_last_ns;
            tiny_events_set( &handle->frames.events, FD_EVENT_TX_DATA );
            if ( ((handle->frames.last_ns + 1) & handle->frames.seq_bits) != handle->frames.confirm_ns )
            {
                LOG( TINY_LOG_DEB, "[%p] PUT frame success\n", handle );
                tiny_events_set( &handle->frames.events, FD_EVENT_I_QUEUE_FREE );
            }
            else
            {
                LOG( TINY_LOG_ERR, "[%p] I_QUEUE is full\n", handle);
            }
        }
        else
        {
            LOG( TINY_LOG_ERR, "[%p] Wrong flag FD_EVENT_I_QUEUE_FREE\n", handle);
        }
        tiny_mutex_unlock( &handle->frames.mutex );
        result = TINY_SUCCESS;
    }
    else
    {
        LOG( TINY_LOG_WRN, "[%p] PUT frame timeout\n", handle );
        result = TINY_ERR_TIMEOUT;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
