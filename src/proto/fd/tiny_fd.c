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

#include "tiny_fd.h"
#include "tiny_fd_int.h"
#include "hal/tiny_types.h"
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

#define HDLC_I_FRAME_BITS 0x00
#define HDLC_I_FRAME_MASK 0x01

#define HDLC_S_FRAME_BITS 0x01
#define HDLC_S_FRAME_MASK 0x03
#define HDLC_S_FRAME_TYPE_REJ 0x04
#define HDLC_S_FRAME_TYPE_RR 0x00
#define HDLC_S_FRAME_TYPE_MASK 0x0C

#define HDLC_U_FRAME_BITS 0x03
#define HDLC_U_FRAME_MASK 0x03
#define HDLC_U_FRAME_TYPE_UA 0x60
#define HDLC_U_FRAME_TYPE_FRMR 0x84
#define HDLC_U_FRAME_TYPE_RSET 0x8C
#define HDLC_U_FRAME_TYPE_SABM 0x2C
#define HDLC_U_FRAME_TYPE_DISC 0x40
#define HDLC_U_FRAME_TYPE_MASK 0xEC

#define HDLC_P_BIT 0x10
#define HDLC_F_BIT 0x10

enum
{
    FD_EVENT_TX_SENDING = 0x01,
    FD_EVENT_TX_DATA_AVAILABLE = 0x02,
    FD_EVENT_QUEUE_HAS_FREE_SLOTS = 0x04,
};

static const uint8_t seq_bits_mask = 0x07;

static int on_frame_read(void *user_data, void *data, int len);
static int on_frame_sent(void *user_data, const void *data, int len);

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

static inline uint8_t __number_of_awaiting_tx_i_frames(tiny_fd_handle_t handle)
{
    return ((uint8_t)(handle->frames.last_ns - handle->frames.confirm_ns) & seq_bits_mask);
}

///////////////////////////////////////////////////////////////////////////////

static inline bool __has_unconfirmed_frames(tiny_fd_handle_t handle)
{
    return (handle->frames.confirm_ns != handle->frames.last_ns);
}

///////////////////////////////////////////////////////////////////////////////

static inline bool __has_non_sent_i_frames(tiny_fd_handle_t handle)
{
    return (handle->frames.last_ns != handle->frames.next_ns);
}

///////////////////////////////////////////////////////////////////////////////

static inline bool __all_frames_are_sent(tiny_fd_handle_t handle)
{
    return (handle->frames.last_ns == handle->frames.next_ns);
}

///////////////////////////////////////////////////////////////////////////////

static inline uint32_t __time_passed_since_last_i_frame(tiny_fd_handle_t handle)
{
    return (uint32_t)(tiny_millis() - handle->frames.last_i_ts);
}

///////////////////////////////////////////////////////////////////////////////

static inline uint32_t __time_passed_since_last_frame_received(tiny_fd_handle_t handle)
{
    return (uint32_t)(tiny_millis() - handle->frames.last_ka_ts);
}

///////////////////////////////////////////////////////////////////////////////

static inline bool __has_non_sent_s_u_frames(tiny_fd_handle_t handle)
{
    return handle->s_u_frames.queue_len > 0;
}

static inline uint8_t __get_i_frame_to_send_index(tiny_fd_handle_t handle)
{
    return (handle->frames.next_ns - handle->frames.confirm_ns) & seq_bits_mask;
}

///////////////////////////////////////////////////////////////////////////////

static bool __put_u_s_frame_to_tx_queue(tiny_fd_handle_t handle, const void *data, int len)
{
    if ( handle->s_u_frames.queue_len < TINY_FD_U_QUEUE_MAX_SIZE )
    {
        uint8_t index = handle->s_u_frames.queue_ptr + handle->s_u_frames.queue_len;
        if ( index >= TINY_FD_U_QUEUE_MAX_SIZE )
            index -= TINY_FD_U_QUEUE_MAX_SIZE;
        handle->s_u_frames.queue[index].len = len;
        memcpy(&handle->s_u_frames.queue[index].u_frame, data, len);
        handle->s_u_frames.queue_len++;
        tiny_events_set(&handle->frames.events, FD_EVENT_TX_DATA_AVAILABLE);
        //        fprintf( stderr, "QUEUE PTR=%d, LEN=%d\n", handle->s_u_frames.queue_ptr, handle->s_u_frames.queue_len
        //        );
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

static void __remove_u_s_frame_from_tx_queue(tiny_fd_handle_t handle)
{
    handle->s_u_frames.queue_ptr++;
    if ( handle->s_u_frames.queue_ptr >= TINY_FD_U_QUEUE_MAX_SIZE )
        handle->s_u_frames.queue_ptr -= TINY_FD_U_QUEUE_MAX_SIZE;
    handle->s_u_frames.queue_len--;
}

///////////////////////////////////////////////////////////////////////////////

static bool __put_i_frame_to_tx_queue(tiny_fd_handle_t handle, const void *data, int len)
{
    uint8_t busy_slots = __number_of_awaiting_tx_i_frames(handle);
    // Check if space is actually available
    if ( busy_slots < handle->frames.max_i_frames )
    {
        uint8_t free_slot = busy_slots + handle->frames.head_ptr;
        while ( free_slot >= handle->frames.max_i_frames )
            free_slot -= handle->frames.max_i_frames;

        handle->frames.i_frames[free_slot]->len = len;
        memcpy(&handle->frames.i_frames[free_slot]->user_payload, data, len);
        handle->frames.last_ns = (handle->frames.last_ns + 1) & seq_bits_mask;
        tiny_events_set(&handle->frames.events, FD_EVENT_TX_DATA_AVAILABLE);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

static int __check_received_frame(tiny_fd_handle_t handle, uint8_t ns)
{
    int result = TINY_SUCCESS;
    if ( ns == handle->frames.next_nr )
    {
        // this is what, we've been waiting for
        // LOG("[%p] Confirming received frame <= %d\n", handle, ns);
        handle->frames.next_nr = (handle->frames.next_nr + 1) & seq_bits_mask;
        handle->frames.sent_reject = 0;
    }
    else
    {
        // definitely we need to send reject. We want to see next_nr frame
        LOG(TINY_LOG_ERR, "[%p] Out of order I-Frame N(s)=%d\n", handle, ns);
        if ( !handle->frames.sent_reject )
        {
            tiny_s_frame_info_t frame = {
                .header.address = 0xFF,
                .header.control = HDLC_S_FRAME_BITS | HDLC_S_FRAME_TYPE_REJ | (handle->frames.next_nr << 5),
            };
            handle->frames.sent_reject = 1;
            __put_u_s_frame_to_tx_queue(handle, &frame, 2);
        }
        result = TINY_ERR_FAILED;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static void __confirm_sent_frames(tiny_fd_handle_t handle, uint8_t nr)
{
    // all frames below nr are received
    while ( nr != handle->frames.confirm_ns )
    {
        if ( handle->frames.confirm_ns == handle->frames.last_ns )
        {
            // TODO: Out of sync
            LOG(TINY_LOG_CRIT, "[%p] Confirmation contains wrong N(r). Remote side is out of sync\n", handle);
            break;
        }
        // LOG("[%p] Confirming sent frames %d\n", handle, handle->frames.confirm_ns);
        if ( handle->on_sent_cb )
        {
            uint8_t i = handle->frames.head_ptr;
            tiny_mutex_unlock(&handle->frames.mutex);
            handle->on_sent_cb(handle->user_data, &handle->frames.i_frames[i]->user_payload,
                               handle->frames.i_frames[i]->len);
            tiny_mutex_lock(&handle->frames.mutex);
        }
        handle->frames.confirm_ns = (handle->frames.confirm_ns + 1) & seq_bits_mask;
        handle->frames.head_ptr++;
        if ( handle->frames.head_ptr >= handle->frames.max_i_frames )
            handle->frames.head_ptr -= handle->frames.max_i_frames;
        handle->frames.retries = handle->retries;
        // Unblock tx queue to allow application to put new frames for sending
        tiny_events_set(&handle->frames.events, FD_EVENT_QUEUE_HAS_FREE_SLOTS);
    }
    LOG(TINY_LOG_DEB, "[%p] Last confirmed frame: %02X\n", handle, handle->frames.confirm_ns);
    // LOG("[%p] N(S)=%d, N(R)=%d\n", handle, handle->frames.confirm_ns, handle->frames.next_nr);
}

///////////////////////////////////////////////////////////////////////////////

static void __resend_all_unconfirmed_frames(tiny_fd_handle_t handle, uint8_t control, uint8_t nr)
{
    // First, we need to check if that is possible. Maybe remote side is not in sync
    while ( handle->frames.next_ns != nr )
    {
        if ( handle->frames.confirm_ns == handle->frames.next_ns )
        {
            // consider here that remote side is not in sync, we cannot perform request
            LOG(TINY_LOG_CRIT, "[%p] Remote side not in sync\n", handle);
            tiny_u_frame_info_t frame = {
                .header.address = 0xFF,
                .header.control = HDLC_U_FRAME_TYPE_FRMR | HDLC_F_BIT | HDLC_U_FRAME_BITS,
                .data1 = control,
                .data2 = (handle->frames.next_nr << 5) | (handle->frames.next_ns << 1),
            };
            // Send 2-byte header + 2 extra bytes
            __put_u_s_frame_to_tx_queue(handle, &frame, 4);
            break;
        }
        handle->frames.next_ns = (handle->frames.next_ns - 1) & seq_bits_mask;
    }
    LOG(TINY_LOG_DEB, "[%p] N(s) is set to %02X\n", handle, handle->frames.next_ns);
    tiny_events_set(&handle->frames.events, FD_EVENT_TX_DATA_AVAILABLE);
}

///////////////////////////////////////////////////////////////////////////////

static void __switch_to_connected_state(tiny_fd_handle_t handle)
{
    if ( handle->state != TINY_FD_STATE_CONNECTED_ABM )
    {
        handle->state = TINY_FD_STATE_CONNECTED_ABM;
        handle->frames.confirm_ns = 0;
        handle->frames.last_ns = 0;
        handle->frames.next_ns = 0;
        handle->frames.next_nr = 0;
        handle->frames.sent_nr = 0;
        handle->frames.sent_reject = 0;
        handle->frames.head_ptr = 0;
        handle->frames.last_ka_ts = tiny_millis();
        tiny_events_set(&handle->frames.events, FD_EVENT_QUEUE_HAS_FREE_SLOTS);
        tiny_events_set(&handle->frames.events, FD_EVENT_TX_DATA_AVAILABLE);
        LOG(TINY_LOG_INFO, "[%p] ABM connection is established\n", handle);
    }
}

///////////////////////////////////////////////////////////////////////////////

static void __switch_to_disconnected_state(tiny_fd_handle_t handle)
{
    if ( handle->state != TINY_FD_STATE_DISCONNECTED )
    {
        handle->state = TINY_FD_STATE_DISCONNECTED;
        handle->frames.confirm_ns = 0;
        handle->frames.last_ns = 0;
        handle->frames.next_ns = 0;
        handle->frames.next_nr = 0;
        handle->frames.sent_nr = 0;
        handle->frames.sent_reject = 0;
        handle->frames.head_ptr = 0;
        tiny_events_clear(&handle->frames.events, FD_EVENT_QUEUE_HAS_FREE_SLOTS);
        LOG(TINY_LOG_INFO, "[%p] Disconnected\n", handle);
    }
}

///////////////////////////////////////////////////////////////////////////////

static int __on_i_frame_read(tiny_fd_handle_t handle, void *data, int len)
{
    uint8_t control = ((uint8_t *)data)[1];
    uint8_t nr = control >> 5;
    uint8_t ns = (control >> 1) & 0x07;
    LOG(TINY_LOG_INFO, "[%p] Receiving I-Frame N(R)=%02X,N(S)=%02X\n", handle, nr, ns);
    int result = __check_received_frame(handle, ns);
    __confirm_sent_frames(handle, nr);
    // Provide data to user only if we expect this frame
    if ( result == TINY_SUCCESS )
    {
        if ( handle->on_frame_cb )
        {
            tiny_mutex_unlock(&handle->frames.mutex);
            handle->on_frame_cb(handle->user_data, (uint8_t *)data + 2, len - 2);
            tiny_mutex_lock(&handle->frames.mutex);
        }
        // Decide whenever we need to send RR after user callback
        // Check if we need to send confirmations separately. If we have something to send, just skip RR S-frame.
        // Also at this point, since we received expected frame, sent_reject will be cleared to 0.
        if ( __all_frames_are_sent(handle) && handle->frames.sent_nr != handle->frames.next_nr )
        {
            tiny_s_frame_info_t frame = {
                .header.address = 0xFF,
                .header.control = HDLC_S_FRAME_BITS | HDLC_S_FRAME_TYPE_RR | (handle->frames.next_nr << 5),
            };
            __put_u_s_frame_to_tx_queue(handle, &frame, 2);
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static int __on_s_frame_read(tiny_fd_handle_t handle, void *data, int len)
{
    uint8_t control = ((uint8_t *)data)[1];
    uint8_t nr = control >> 5;
    int result = TINY_ERR_FAILED;
    LOG(TINY_LOG_INFO, "[%p] Receiving S-Frame N(R)=%02X, type=%s\n", handle, nr,
        ((control >> 2) & 0x03) == 0x00 ? "RR" : "REJ");
    if ( (control & HDLC_S_FRAME_TYPE_MASK) == HDLC_S_FRAME_TYPE_REJ )
    {
        __confirm_sent_frames(handle, nr);
        __resend_all_unconfirmed_frames(handle, control, nr);
    }
    else if ( (control & HDLC_S_FRAME_TYPE_MASK) == HDLC_S_FRAME_TYPE_RR )
    {
        __confirm_sent_frames(handle, nr);
        if ( control & HDLC_P_BIT )
        {
            // Send answer if we don't have frames to send
            if ( handle->frames.next_ns == handle->frames.last_ns )
            {
                tiny_s_frame_info_t frame = {
                    .header.address = 0xFF,
                    .header.control = HDLC_S_FRAME_BITS | HDLC_S_FRAME_TYPE_RR | (handle->frames.next_nr << 5),
                };
                __put_u_s_frame_to_tx_queue(handle, &frame, 2);
            }
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

static int __on_u_frame_read(tiny_fd_handle_t handle, void *data, int len)
{
    uint8_t control = ((uint8_t *)data)[1];
    uint8_t type = control & HDLC_U_FRAME_TYPE_MASK;
    int result = TINY_ERR_FAILED;
    LOG(TINY_LOG_INFO, "[%p] Receiving U-Frame type=%02X\n", handle, type);
    if ( type == HDLC_U_FRAME_TYPE_SABM )
    {
        tiny_u_frame_info_t frame = {
            .header.address = 0xFF,
            .header.control = HDLC_U_FRAME_TYPE_UA | HDLC_F_BIT | HDLC_U_FRAME_BITS,
        };
        __put_u_s_frame_to_tx_queue(handle, &frame, 2);
        __switch_to_connected_state(handle);
    }
    else if ( type == HDLC_U_FRAME_TYPE_DISC )
    {
        tiny_u_frame_info_t frame = {
            .header.address = 0xFF,
            .header.control = HDLC_U_FRAME_TYPE_UA | HDLC_F_BIT | HDLC_U_FRAME_BITS,
        };
        __put_u_s_frame_to_tx_queue(handle, &frame, 2);
        __switch_to_disconnected_state(handle);
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
        if ( handle->state == TINY_FD_STATE_CONNECTING )
        {
            // confirmation received
            __switch_to_connected_state(handle);
        }
        else if ( handle->state == TINY_FD_STATE_DISCONNECTING )
        {
            __switch_to_disconnected_state(handle);
        }
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
    // printf("[%p] Incoming frame of size %i\n", handle, len);
    handle->frames.last_ka_ts = tiny_millis();
    if ( len < 2 )
    {
        LOG(TINY_LOG_WRN, "FD: received too small frame\n");
        return TINY_ERR_FAILED;
    }
    tiny_mutex_lock(&handle->frames.mutex);
    handle->frames.ka_confirmed = 1;
    uint8_t control = ((uint8_t *)data)[1];
    if ( (control & HDLC_U_FRAME_MASK) == HDLC_U_FRAME_MASK )
    {
        __on_u_frame_read(handle, data, len);
    }
    else if ( handle->state != TINY_FD_STATE_CONNECTED_ABM && handle->state != TINY_FD_STATE_DISCONNECTING )
    {
        // Should send DM in case we receive here S- or I-frames.
        // If connection is not established, we should ignore all frames except U-frames
        LOG(TINY_LOG_ERR, "[%p] ABM connection is not established\n", handle);
        tiny_u_frame_info_t frame = {
            .header.address = 0xFF,
            .header.control = HDLC_P_BIT | HDLC_U_FRAME_TYPE_SABM | HDLC_U_FRAME_BITS,
        };
        __put_u_s_frame_to_tx_queue(handle, &frame, 2);
        handle->state = TINY_FD_STATE_CONNECTING;
    }
    else if ( (control & HDLC_I_FRAME_MASK) == HDLC_I_FRAME_BITS )
    {
        __on_i_frame_read(handle, data, len);
    }
    else if ( (control & HDLC_S_FRAME_MASK) == HDLC_S_FRAME_BITS )
    {
        __on_s_frame_read(handle, data, len);
    }
    else
    {
        LOG(TINY_LOG_WRN, "[%p] Unknown hdlc frame received\n", handle);
    }
    tiny_mutex_unlock(&handle->frames.mutex);
    return len;
}

static int on_frame_sent(void *user_data, const void *data, int len)
{
    tiny_fd_handle_t handle = (tiny_fd_handle_t)user_data;
    uint8_t control = ((uint8_t *)data)[1];
    tiny_mutex_lock(&handle->frames.mutex);
    if ( (control & HDLC_I_FRAME_MASK) == HDLC_I_FRAME_BITS )
    {
        // nothing to do
        // we need to wait for confirmation from remote side
    }
    else if ( (control & HDLC_S_FRAME_MASK) == HDLC_S_FRAME_BITS )
    {
        __remove_u_s_frame_from_tx_queue(handle);
        //        fprintf( stderr, "QUEUE PTR=%d, LEN=%d\n", handle->s_u_frames.queue_ptr, handle->s_u_frames.queue_len
        //        );
    }
    else if ( (control & HDLC_U_FRAME_MASK) == HDLC_U_FRAME_BITS )
    {
        __remove_u_s_frame_from_tx_queue(handle);
        //        fprintf( stderr, "QUEUE PTR=%d, LEN=%d\n", handle->s_u_frames.queue_ptr, handle->s_u_frames.queue_len
        //        );
    }
    tiny_mutex_unlock(&handle->frames.mutex);
    tiny_events_clear(&handle->frames.events, FD_EVENT_TX_SENDING);
    return len;
}

///////////////////////////////////////////////////////////////////////////////

static int tiny_fd_calculate_mtu_size(int buffer_size, int window, hdlc_crc_t crc_type)
{
    return (buffer_size -
            sizeof(tiny_fd_data_t)
            // RX overhead
            - sizeof(hdlc_ll_data_t) - sizeof(tiny_frame_header_t) -
            get_crc_field_size(crc_type)
            // TX overhead
            - window * (sizeof(tiny_i_frame_info_t *) + sizeof(tiny_i_frame_info_t) -
                        sizeof(((tiny_i_frame_info_t *)0)->user_payload))) /
           (window + 1);
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_init(tiny_fd_handle_t *handle, tiny_fd_init_t *init)
{
    *handle = NULL;
    if ( (0 == init->on_frame_cb) || (0 == init->buffer) || (0 == init->buffer_size) )
    {
        return TINY_ERR_FAILED;
    }
    if ( init->mtu == 0 )
    {
        init->mtu = tiny_fd_calculate_mtu_size(init->buffer_size, init->window_frames, init->crc_type);
        if ( init->mtu < 1 )
        {
            LOG(TINY_LOG_CRIT, "Calculated mtu size is zero, no payload transfer is available\n");
            return TINY_ERR_INVALID_DATA;
        }
    }
    if ( init->buffer_size < tiny_fd_buffer_size_by_mtu_ex(init->mtu, init->window_frames, init->crc_type) )
    {
        LOG(TINY_LOG_CRIT, "Too small buffer for FD protocol %i < %i\n", init->buffer_size,
            tiny_fd_buffer_size_by_mtu_ex(init->mtu, init->window_frames, init->crc_type));
        return TINY_ERR_INVALID_DATA;
    }
    if ( init->window_frames > 7 )
    {
        LOG(TINY_LOG_CRIT, "HDLC doesn't support more than 7-frames queue\n");
        return TINY_ERR_INVALID_DATA;
    }
    if ( init->window_frames < 2 )
    {
        LOG(TINY_LOG_CRIT, "HDLC doesn't support less than 2-frames queue\n");
        return TINY_ERR_INVALID_DATA;
    }
    if ( !init->retry_timeout && !init->send_timeout )
    {
        LOG(TINY_LOG_CRIT, "HDLC uses timeouts for ACK, at least retry_timeout, or send_timeout must be specified\n");
        return TINY_ERR_INVALID_DATA;
    }
    memset(init->buffer, 0, init->buffer_size);

    uint8_t *ptr = (uint8_t *)init->buffer;
    /* Lets locate main FD protocol data at the beginning of specified buffer */
    tiny_fd_data_t *protocol = (tiny_fd_data_t *)ptr;
    ptr += sizeof(tiny_fd_data_t);
    /* Next we need some space to hold pointers to tiny_i_frame_info_t records (window_frames pointers) */
    protocol->frames.i_frames = (tiny_i_frame_info_t **)(ptr);
    ptr += sizeof(tiny_i_frame_info_t *) * init->window_frames;
    protocol->frames.max_i_frames = init->window_frames;
    protocol->frames.mtu = init->mtu;
    /* Lets allocate memory for TX frames, we have <window_frames> TX frames */
    for ( int i = 0; i < init->window_frames; i++ )
    {
        protocol->frames.i_frames[i] = (tiny_i_frame_info_t *)ptr;
        /* We do not allocate space for CRC field since, it is calculated only during send operation by HDLC low level
         */
        ptr += protocol->frames.mtu + sizeof(tiny_i_frame_info_t) - sizeof(((tiny_i_frame_info_t *)0)->user_payload);
    }
    /* Lets allocate memory for HDLC low level protocol */
    hdlc_ll_init_t _init = { 0 };
    _init.on_frame_read = on_frame_read;
    _init.on_frame_sent = on_frame_sent;
    _init.user_data = protocol;
    _init.crc_type = init->crc_type;
    _init.buf_size = hdlc_ll_get_buf_size_ex(protocol->frames.mtu + sizeof(tiny_frame_header_t), init->crc_type);
    _init.buf = ptr;
    ptr += _init.buf_size;
    if ( ptr > (uint8_t *)init->buffer + init->buffer_size )
    {
        LOG(TINY_LOG_CRIT, "Out of provided memory: provided %i bytes, used %i bytes\n", init->buffer_size,
            (int)(ptr - (uint8_t *)init->buffer));
        return TINY_ERR_INVALID_DATA;
    }

    int result = hdlc_ll_init(&protocol->_hdlc, &_init);
    if ( result != TINY_SUCCESS )
    {
        LOG(TINY_LOG_CRIT, "HDLC low level initialization failed");
        return result;
    }

    protocol->user_data = init->pdata;
    protocol->on_frame_cb = init->on_frame_cb;
    protocol->on_sent_cb = init->on_sent_cb;
    protocol->send_timeout = init->send_timeout;
    protocol->ka_timeout = 5000;
    protocol->retry_timeout =
        init->retry_timeout ? init->retry_timeout : (protocol->send_timeout / (init->retries + 1));
    protocol->retries = init->retries;
    protocol->frames.retries = init->retries;
    protocol->state = TINY_FD_STATE_DISCONNECTED;

    tiny_mutex_create(&protocol->frames.mutex);
    tiny_events_create(&protocol->frames.events);
    *handle = protocol;

    // Request remote side for ABM
    // TODO: No need?
    /*    tiny_u_frame_info_t frame = {
            .header.address = 0xFF,
            .header.control = HDLC_P_BIT | HDLC_U_FRAME_TYPE_SABM | HDLC_U_FRAME_BITS,
        };
        __put_u_s_frame_to_tx_queue( *handle, &frame, 2 ); */
    return TINY_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_fd_close(tiny_fd_handle_t handle)
{
    hdlc_ll_close(handle->_hdlc);
    tiny_events_destroy(&handle->frames.events);
    tiny_mutex_destroy(&handle->frames.mutex);
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_on_rx_data(tiny_fd_handle_t handle, const void *data, int len)
{
    const uint8_t *ptr = (const uint8_t *)data;
    while ( len )
    {
        int error;
        int processed_bytes = hdlc_ll_run_rx(handle->_hdlc, ptr, len, &error);
        if ( error == TINY_ERR_WRONG_CRC )
        {
            LOG(TINY_LOG_WRN, "[%p] HDLC CRC sum mismatch\n", handle);
        }
        ptr += processed_bytes;
        len -= processed_bytes;
    }
    return TINY_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_run_rx(tiny_fd_handle_t handle, read_block_cb_t read_func)
{
    uint8_t buf[4];
    int len = read_func(handle->user_data, buf, sizeof(buf));
    if ( len <= 0 )
    {
        return len;
    }
    return tiny_fd_on_rx_data(handle, buf, len);
}

///////////////////////////////////////////////////////////////////////////////

static uint8_t *tiny_fd_get_next_frame_to_send(tiny_fd_handle_t handle, int *len)
{
    uint8_t *data = NULL;
    // Tx data available
    tiny_mutex_lock(&handle->frames.mutex);
    if ( __has_non_sent_s_u_frames(handle) )
    {
        // clear queue only, when send is done, so for now, use pointer data for sending only
        data = (uint8_t *)&handle->s_u_frames.queue[handle->s_u_frames.queue_ptr].u_frame;
        *len = handle->s_u_frames.queue[handle->s_u_frames.queue_ptr].len;

#if TINY_FD_DEBUG
        if ( (data[1] & HDLC_U_FRAME_MASK) == HDLC_U_FRAME_BITS )
        {
            LOG(TINY_LOG_INFO, "[%p] Sending U-Frame type=%02X\n", handle, data[1] & HDLC_U_FRAME_TYPE_MASK);
        }
        else if ( (data[1] & HDLC_S_FRAME_MASK) == HDLC_S_FRAME_BITS )
        {
            LOG(TINY_LOG_INFO, "[%p] Sending S-Frame N(R)=%02X, type=%s\n", handle, data[1] >> 5,
                ((data[1] >> 2) & 0x03) == 0x00 ? "RR" : "REJ");
        }
#endif
    }
    else if ( __has_non_sent_i_frames(handle) && ( handle->state == TINY_FD_STATE_CONNECTED_ABM || handle->state == TINY_FD_STATE_DISCONNECTING ) )
    {
        uint8_t i = __get_i_frame_to_send_index(handle) + handle->frames.head_ptr;
        while ( i >= handle->frames.max_i_frames )
            i -= handle->frames.max_i_frames;

        data = (uint8_t *)&handle->frames.i_frames[i]->header;
        *len = handle->frames.i_frames[i]->len + sizeof(tiny_frame_header_t);
        handle->frames.i_frames[i]->header.address = 0xFF;
        handle->frames.i_frames[i]->header.control = (handle->frames.next_ns << 1) | (handle->frames.next_nr << 5);
        LOG(TINY_LOG_INFO, "[%p] Sending I-Frame N(R)=%02X,N(S)=%02X\n", handle, handle->frames.next_nr,
            handle->frames.next_ns);
        handle->frames.next_ns++;
        handle->frames.next_ns &= seq_bits_mask;
        // Move to different place
        handle->frames.sent_nr = handle->frames.next_nr;
        handle->frames.last_i_ts = tiny_millis();
        handle->frames.last_ka_ts = tiny_millis();
    }
    tiny_mutex_unlock(&handle->frames.mutex);
    return data;
}

///////////////////////////////////////////////////////////////////////////////

static void tiny_fd_connected_on_idle_timeout(tiny_fd_handle_t handle)
{
    tiny_mutex_lock(&handle->frames.mutex);
    if ( __has_unconfirmed_frames(handle) && __all_frames_are_sent(handle) &&
         __time_passed_since_last_i_frame(handle) >= handle->retry_timeout )
    {
        // if sent frame was not confirmed due to noisy line
        if ( handle->frames.retries > 0 )
        {
            LOG(TINY_LOG_WRN,
                "[%p] Timeout, resending unconfirmed frames: last(%" PRIu32 " ms, now(%" PRIu32 " ms), timeout(%" PRIu32
                " ms))\n",
                handle, handle->frames.last_i_ts, tiny_millis(), handle->retry_timeout);
            handle->frames.retries--;
            // Do not use mutex for confirm_ns value as it is byte-value
            __resend_all_unconfirmed_frames(handle, 0, handle->frames.confirm_ns);
        }
        else
        {
            LOG(TINY_LOG_CRIT, "[%p] Remote side not responding, flushing I-frames\n", handle);
            __switch_to_disconnected_state(handle);
        }
    }
    else if ( __time_passed_since_last_frame_received(handle) > handle->ka_timeout )
    {
        if ( !handle->frames.ka_confirmed )
        {
            LOG(TINY_LOG_CRIT, "[%p] No keep alive after timeout\n", handle);
            __switch_to_disconnected_state(handle);
        }
        else
        {
            // Nothing to send, all frames are confirmed, just send keep alive
            tiny_s_frame_info_t frame = {
                .header.address = 0xFF,
                .header.control = HDLC_S_FRAME_BITS | HDLC_S_FRAME_TYPE_RR | (handle->frames.next_nr << 5) | HDLC_P_BIT,
            };
            handle->frames.ka_confirmed = 0;
            __put_u_s_frame_to_tx_queue(handle, &frame, 2);
        }
        handle->frames.last_ka_ts = tiny_millis();
    }
    tiny_mutex_unlock(&handle->frames.mutex);
}

///////////////////////////////////////////////////////////////////////////////

static void tiny_fd_disconnected_on_idle_timeout(tiny_fd_handle_t handle)
{
    tiny_mutex_lock(&handle->frames.mutex);
    if ( __time_passed_since_last_frame_received(handle) >= handle->retry_timeout ||
         __number_of_awaiting_tx_i_frames(handle) > 0 )
    {
        LOG(TINY_LOG_ERR, "[%p] ABM connection is not established\n", handle);
        // Try to establish ABM connection
        tiny_u_frame_info_t frame = {
            .header.address = 0xFF,
            .header.control = HDLC_P_BIT | HDLC_U_FRAME_TYPE_SABM | HDLC_U_FRAME_BITS,
        };
        __put_u_s_frame_to_tx_queue(handle, &frame, 2);
        handle->frames.last_ka_ts = tiny_millis();
    }
    tiny_mutex_unlock(&handle->frames.mutex);
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_get_tx_data(tiny_fd_handle_t handle, void *data, int len)
{
    bool repeat = true;
    int result = 0;
    while ( result < len )
    {
        int generated_data = 0;
        if ( handle->state == TINY_FD_STATE_CONNECTED_ABM || handle->state == TINY_FD_STATE_DISCONNECTING )
        {
            tiny_fd_connected_on_idle_timeout(handle);
        }
        else
        {
            tiny_fd_disconnected_on_idle_timeout(handle);
        }
        // Check if send on hdlc level operation is in progress and do some work
        if ( tiny_events_wait(&handle->frames.events, FD_EVENT_TX_SENDING, EVENT_BITS_LEAVE, 0) )
        {
            generated_data = hdlc_ll_run_tx(handle->_hdlc, ((uint8_t *)data) + result, len - result);
        }
        // Since no send operation is in progress, check if we have something to send
        else if ( tiny_events_wait(&handle->frames.events, FD_EVENT_TX_DATA_AVAILABLE, EVENT_BITS_CLEAR, 0) )
        {
            int frame_len = 0;
            uint8_t *frame_data = tiny_fd_get_next_frame_to_send(handle, &frame_len);
            if ( frame_data != NULL )
            {
                // Force to check for new frame once again
                tiny_events_set(&handle->frames.events, FD_EVENT_TX_DATA_AVAILABLE);
                tiny_events_set(&handle->frames.events, FD_EVENT_TX_SENDING);
                // Do not use timeout for hdlc_send(), as hdlc level is ready to accept next frame
                // (FD_EVENT_TX_SENDING is not set). And at this step we do not need hdlc_send() to
                // send data.
                hdlc_ll_put(handle->_hdlc, frame_data, frame_len);
            }
        }
        result += generated_data;
        if ( !generated_data )
        {
            if ( !repeat )
            {
                break;
            }
            repeat = false;
        }
        else
        {
            repeat = true;
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_run_tx(tiny_fd_handle_t handle, write_block_cb_t write_func)
{
    uint8_t buf[4];
    int len = tiny_fd_get_tx_data(handle, buf, sizeof(buf));
    if ( len <= 0 )
    {
        return len;
    }
    uint8_t *ptr = buf;
    while ( len )
    {
        int result = write_func(handle->user_data, ptr, len);
        if ( result < 0 )
        {
            return result;
        }
        len -= result;
        ptr += result;
    }
    return TINY_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_send_packet(tiny_fd_handle_t handle, const void *data, int len)
{
    int result;
    LOG(TINY_LOG_DEB, "[%p] PUT frame\n", handle);
    // Check frame size againts mtu
    // MTU doesn't include header and crc fields, only user payload
    if ( len > handle->frames.mtu )
    {
        LOG(TINY_LOG_ERR, "[%p] PUT frame error\n", handle);
        result = TINY_ERR_DATA_TOO_LARGE;
    }
    // Wait until there is room for new frame
    else if ( tiny_events_wait(&handle->frames.events, FD_EVENT_QUEUE_HAS_FREE_SLOTS, EVENT_BITS_CLEAR,
                               handle->send_timeout) )
    {
        tiny_mutex_lock(&handle->frames.mutex);
        // Check if space is actually available
        if ( __put_i_frame_to_tx_queue(handle, data, len) )
        {
            if ( __number_of_awaiting_tx_i_frames(handle) < handle->frames.max_i_frames )
            {
                LOG(TINY_LOG_INFO, "[%p] I_QUEUE is N(S)queue=%d, N(S)confirm=%d, N(S)next=%d\n", handle,
                    handle->frames.last_ns, handle->frames.confirm_ns, handle->frames.next_ns);
                tiny_events_set(&handle->frames.events, FD_EVENT_QUEUE_HAS_FREE_SLOTS);
            }
            else
            {
                LOG(TINY_LOG_ERR, "[%p] I_QUEUE is full N(S)queue=%d, N(S)confirm=%d, N(S)next=%d\n", handle,
                    handle->frames.last_ns, handle->frames.confirm_ns, handle->frames.next_ns);
            }
            result = TINY_SUCCESS;
        }
        else
        {
            result = TINY_ERR_TIMEOUT;
            LOG(TINY_LOG_ERR, "[%p] Wrong flag FD_EVENT_QUEUE_HAS_FREE_SLOTS\n", handle);
        }
        tiny_mutex_unlock(&handle->frames.mutex);
    }
    else
    {
        LOG(TINY_LOG_WRN, "[%p] PUT frame timeout\n", handle);
        result = TINY_ERR_TIMEOUT;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_buffer_size_by_mtu(int mtu, int window)
{
    return tiny_fd_buffer_size_by_mtu_ex(mtu, window, HDLC_CRC_16);
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_buffer_size_by_mtu_ex(int mtu, int window, hdlc_crc_t crc_type)
{
    return sizeof(tiny_fd_data_t) +
           // RX side
           hdlc_ll_get_buf_size_ex(mtu + sizeof(tiny_frame_header_t), crc_type) +
           // TX side
           (sizeof(tiny_i_frame_info_t *) + sizeof(tiny_i_frame_info_t) + mtu -
            sizeof(((tiny_i_frame_info_t *)0)->user_payload)) *
               window;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_fd_set_ka_timeout(tiny_fd_handle_t handle, uint32_t keep_alive)
{
    handle->ka_timeout = keep_alive;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_get_mtu(tiny_fd_handle_t handle)
{
    return handle->frames.mtu;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_send(tiny_fd_handle_t handle, const void *data, int len)
{
    const uint8_t *ptr = (const uint8_t *)data;
    int left = len;
    while ( left > 0 )
    {
        int size = left < handle->frames.mtu ? left : handle->frames.mtu;
        int result = tiny_fd_send_packet(handle, ptr, size);
        if ( result != TINY_SUCCESS )
        {
            break;
        }
        left -= result;
    }
    return left;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_get_status(tiny_fd_handle_t handle)
{
    if ( !handle )
    {
         return TINY_ERR_INVALID_DATA;
    }
    int result = TINY_ERR_FAILED;
    tiny_mutex_lock(&handle->frames.mutex);
    if ( (handle->state == TINY_FD_STATE_CONNECTED_ABM) || (handle->state == TINY_FD_STATE_DISCONNECTING) )
    {
        result = TINY_SUCCESS;
    }
    tiny_mutex_unlock(&handle->frames.mutex);
    return result;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_fd_disconnect(tiny_fd_handle_t handle)
{
    if ( !handle )
    {
         return TINY_ERR_INVALID_DATA;
    }
    int result = TINY_SUCCESS;
    tiny_mutex_lock(&handle->frames.mutex);
    tiny_u_frame_info_t frame = {
        .header.address = 0xFF,
        .header.control = HDLC_U_FRAME_TYPE_DISC | HDLC_P_BIT | HDLC_U_FRAME_BITS,
    };
    if ( !__put_u_s_frame_to_tx_queue(handle, &frame, 2) )
    {
        result = TINY_ERR_FAILED;
    }
    else
    {
        handle->state = TINY_FD_STATE_DISCONNECTING;
    }
    tiny_mutex_unlock(&handle->frames.mutex);
    return result;
}

///////////////////////////////////////////////////////////////////////////////

