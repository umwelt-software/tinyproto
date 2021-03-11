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

#include "hdlc.h"
#include "hdlc_int.h"
#include "proto/crc/crc.h"
#include "hal/tiny_debug.h"

#include <stddef.h>

#ifndef TINY_HDLC_DEBUG
#define TINY_HDLC_DEBUG 0
#endif

#if TINY_HDLC_DEBUG
#define LOG(...)  TINY_LOG(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define FLAG_SEQUENCE            0x7E
#define FILL_BYTE                0xFF
#define TINY_ESCAPE_CHAR         0x7D
#define TINY_ESCAPE_BIT          0x20

enum
{
    TX_ACCEPT_BIT = 0x01,
    TX_DATA_READY_BIT = 0x02,
    TX_DATA_SENT_BIT = 0x04,
    RX_DATA_READY_BIT = 0x08,
};

static int hdlc_ll_read_start( hdlc_ll_handle_t handle, const uint8_t *data, int len );
static int hdlc_ll_read_data( hdlc_ll_handle_t handle, const uint8_t *data, int len );
static int hdlc_ll_read_end( hdlc_ll_handle_t handle, const uint8_t *data, int len );

static int hdlc_ll_send_start( hdlc_ll_handle_t handle );
static int hdlc_ll_send_data( hdlc_ll_handle_t handle );
static int hdlc_ll_send_tx_internal( hdlc_ll_handle_t handle, const void *data, int len );
static int hdlc_ll_send_crc( hdlc_ll_handle_t handle );
static int hdlc_ll_send_end( hdlc_ll_handle_t handle );

////////////////////////////////////////////////////////////////////////////////////////////

int hdlc_ll_init( hdlc_ll_handle_t * handle, hdlc_ll_init_t *init )
{
    *handle = NULL;
    if ( !init->buf || init->buf_size < sizeof(hdlc_ll_data_t) )
    {
        LOG(TINY_LOG_ERR, "[HDLC] failed to init hdlc. buf=%p, size=%i (%i required)\n", init->buf, init->buf_size,
            (int)sizeof(hdlc_ll_data_t) );
        return TINY_ERR_FAILED;
    }
    *handle = (hdlc_ll_handle_t)init->buf;
    (*handle)->rx_buf = (uint8_t *)init->buf + sizeof(hdlc_ll_data_t);
    (*handle)->rx_buf_size = init->buf_size - sizeof(hdlc_ll_data_t);
    (*handle)->crc_type = init->crc_type == HDLC_CRC_OFF ? 0: init->crc_type;
    (*handle)->on_frame_read = init->on_frame_read;
    (*handle)->on_frame_sent = init->on_frame_sent;
    (*handle)->user_data = init->user_data;

    // Must be last
    hdlc_ll_reset( *handle, HDLC_LL_RESET_BOTH );
    return TINY_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////

int hdlc_ll_close( hdlc_ll_handle_t handle )
{
    if ( handle && handle->tx.data )
    {
        if ( handle->on_frame_sent )
        {
            handle->on_frame_sent( handle->user_data, handle->tx.origin_data,
                                   (int)(handle->tx.data - handle->tx.origin_data) );
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////

void hdlc_ll_reset( hdlc_ll_handle_t handle, uint8_t flags )
{
    if ( flags != HDLC_LL_RESET_TX_ONLY )
    {
        handle->rx.state = hdlc_ll_read_start;
    }
    if ( flags != HDLC_LL_RESET_RX_ONLY )
    {
        handle->tx.data = NULL;
        handle->tx.origin_data = NULL;
        handle->tx.escape = 0;
        handle->tx.state = hdlc_ll_send_start;
    }
}

////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_send_start( hdlc_ll_handle_t handle )
{
    // Do not clear data ready bit here in case if 0x7F is failed to be sent
    if ( !handle->tx.origin_data )
    {
        LOG(TINY_LOG_DEB, "[HDLC:%p] SENDING START NO DATA READY\n", handle);
        return 0;
    }
    LOG(TINY_LOG_INFO, "[HDLC:%p] Starting send op for HDLC frame\n", handle);
    switch (handle->crc_type)
    {
#ifdef CONFIG_ENABLE_FCS16
        case HDLC_CRC_16:
            handle->tx.crc = crc16( PPPINITFCS16, handle->tx.data, handle->tx.len ); break;
#endif
#ifdef CONFIG_ENABLE_FCS32
        case HDLC_CRC_32:
            handle->tx.crc = crc32( PPPINITFCS32, handle->tx.data, handle->tx.len ); break;
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
        case HDLC_CRC_8:
            handle->tx.crc = chksum( INITCHECKSUM, handle->tx.data, handle->tx.len ); break;
#endif
        default: break;
    }

    uint8_t buf[1] = { FLAG_SEQUENCE };
    int result = hdlc_ll_send_tx_internal( handle, buf, sizeof(buf) );
    if ( result == 1 )
    {
        LOG(TINY_LOG_DEB, "[HDLC:%p] hdlc_ll_send_data\n", handle);
        LOG(TINY_LOG_DEB, "[HDLC:%p] TX: %02X\n", handle, buf[0]);
        handle->tx.state = hdlc_ll_send_data;
        handle->tx.escape = 0;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_send_data( hdlc_ll_handle_t handle )
{
    if ( handle->tx.len == 0 )
    {
        LOG(TINY_LOG_DEB, "[HDLC:%p] hdlc_ll_send_crc\n", handle);
        handle->tx.state = hdlc_ll_send_crc;
        return 0;
    }
    int pos = 0;
    while ( handle->tx.data[pos] != FLAG_SEQUENCE &&
            handle->tx.data[pos] != TINY_ESCAPE_CHAR &&
            pos < handle->tx.len )
    {
        pos++;
    }
    int result = 0;
    if ( pos )
    {
        result = hdlc_ll_send_tx_internal( handle, handle->tx.data, pos );
        if ( result > 0 )
        {
            #if TINY_HDLC_DEBUG
            for(int i=0; i<result; i++) LOG(TINY_LOG_DEB, "[HDLC:%p] TX: %02X\n", handle, handle->tx.data[i]);
            #endif
            handle->tx.data += result;
            handle->tx.len -= result;
        }
    }
    else
    {
        uint8_t buf[1] = { handle->tx.escape ? ( handle->tx.data[0] ^ TINY_ESCAPE_BIT )
                                             : TINY_ESCAPE_CHAR };
        result = hdlc_ll_send_tx_internal( handle, buf, sizeof(buf) );
        if ( result > 0 )
        {
            LOG(TINY_LOG_DEB, "[HDLC:%p] TX: %02X\n", handle, buf[0]);
            handle->tx.escape = !handle->tx.escape;
            if ( !handle->tx.escape )
            {
                handle->tx.data++;
                handle->tx.len--;
            }
        }
    }
    if ( handle->tx.len == 0 )
    {
        LOG(TINY_LOG_DEB, "[HDLC:%p] hdlc_ll_send_crc\n", handle);
        handle->tx.state = hdlc_ll_send_crc;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_send_crc( hdlc_ll_handle_t handle )
{
    int result = 1;
    if ( handle->tx.len == (uint8_t)handle->crc_type )
    {
        handle->tx.state = hdlc_ll_send_end;
    }
    else
    {
        uint8_t byte = handle->tx.crc >> handle->tx.len;
        if ( byte != TINY_ESCAPE_CHAR && byte != FLAG_SEQUENCE )
        {
            result = hdlc_ll_send_tx_internal( handle, &byte, sizeof(byte) );
            if ( result == 1 )
            {
                LOG(TINY_LOG_DEB, "[HDLC:%p] TX: %02X\n", handle, byte);
                handle->tx.len += 8;
            }
        }
        else
        {
            byte = handle->tx.escape ? ( byte ^ TINY_ESCAPE_BIT ) : TINY_ESCAPE_CHAR;
            result = hdlc_ll_send_tx_internal( handle, &byte, sizeof(byte) );
            if ( result == 1 )
            {
                LOG(TINY_LOG_DEB, "[HDLC:%p] TX: %02X\n", handle, byte);
                handle->tx.escape = !handle->tx.escape;
                if ( !handle->tx.escape )
                {
                    handle->tx.len += 8;
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_send_end( hdlc_ll_handle_t handle )
{
    LOG(TINY_LOG_DEB, "[HDLC:%p] hdlc_ll_send_end\n", handle);
    uint8_t buf[1] = { FLAG_SEQUENCE };
    int result = hdlc_ll_send_tx_internal( handle, buf, sizeof(buf) );
    if ( result == 1 )
    {
        LOG(TINY_LOG_DEB, "[HDLC:%p] TX: %02X\n", handle, buf[0]);
        LOG(TINY_LOG_INFO, "[HDLC:%p] hdlc_ll_send_end HDLC send op successful\n", handle);
        handle->tx.state = hdlc_ll_send_start;
        handle->tx.escape = 0;
        int len = (int)(handle->tx.data - handle->tx.origin_data);
        const void *ptr = handle->tx.origin_data;
        handle->tx.origin_data = NULL;
        handle->tx.data = NULL;
        if ( handle->on_frame_sent )
        {
            handle->on_frame_sent( handle->user_data, ptr, len );
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_send_tx_internal( hdlc_ll_handle_t handle, const void *data, int len )
{
    const uint8_t *ptr = (const uint8_t *)data;
    int sent = 0;
    while ( len-- && handle->tx.out_buffer_len )
    {
        handle->tx.out_buffer[0] = ptr[0];
        handle->tx.out_buffer_len--;
        handle->tx.out_buffer++;
        ptr++;
        sent++;
    }
    return sent;
}

////////////////////////////////////////////////////////////////////////////////////////////

int hdlc_ll_run_tx( hdlc_ll_handle_t handle, void *data, int len )
{
    bool repeated_empty_data = false;
    handle->tx.out_buffer = (uint8_t *)data;
    handle->tx.out_buffer_len = len;
    while ( handle->tx.out_buffer_len )
    {
        int result = handle->tx.state( handle );
        if ( result < 0 )
        {
            #if TINY_HDLC_DEBUG
            LOG(TINY_LOG_ERR, "[HDLC:%p] failed to run state with result: %d\n", handle, result );
            #endif
            /*
             * Some error happened. For we do not pass error code to upper layer.
             * Passing error code as the result of this function can confuse user code. So
             * need some other way to do that
             */
            break;
        }
        else if ( result == 0 )
        {
            /*
             * Some tx state functions may return zero result, just in case of switching between states.
             * Exiting in this case can cause tx user buffer to be underfilled. To avoid that, just check
             * twice to ensure that we do not have more data to send.
             */
            if ( repeated_empty_data )
            {
                break;
            }
            repeated_empty_data = true;
        }
        else
        {
            repeated_empty_data = false;
        }
    }
    return len - handle->tx.out_buffer_len;
}

////////////////////////////////////////////////////////////////////////////////////////////

int hdlc_ll_put( hdlc_ll_handle_t handle, const void *data, int len )
{
    LOG(TINY_LOG_DEB, "[HDLC:%p] hdlc_ll_put\n", handle);
    if ( !len || !data || !handle )
    {
        return TINY_ERR_INVALID_DATA;
    }
    // Check if TX thread is ready to accept new data
    if ( handle->tx.origin_data )
    {
        LOG(TINY_LOG_WRN, "[HDLC:%p] hdlc_ll_put FAILED\n", handle);
        return TINY_ERR_BUSY;
    }
    LOG(TINY_LOG_DEB, "[HDLC:%p] hdlc_ll_put SUCCESS\n", handle);
    handle->tx.origin_data = data;
    handle->tx.data = data;
    handle->tx.len = len;
    return TINY_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_read_start( hdlc_ll_handle_t handle, const uint8_t *data, int len )
{
    if ( !len )
    {
        return 0;
    }
    if ( data[0] != FLAG_SEQUENCE )
    {
        if ( data[0] != FILL_BYTE )
        {
            // TODO: Skip byte, but we received some wrong data
        }
        return 1;
    }
    LOG(TINY_LOG_DEB, "[HDLC:%p] RX: %02X\n", handle, data[0]);
    handle->rx.escape = 0;
    handle->rx.data = (uint8_t *)handle->rx_buf;
    handle->rx.state = hdlc_ll_read_data;
    return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_read_data( hdlc_ll_handle_t handle, const uint8_t *data, int len )
{
    int result = 0;
    while ( len > 0 )
    {
        uint8_t byte = data[0];
        LOG(TINY_LOG_DEB, "[HDLC:%p] RX: %02X\n", handle, byte);
        if ( byte == FLAG_SEQUENCE )
        {
            handle->rx.state = hdlc_ll_read_end;
            result++;
            break;
        }
        if ( byte == TINY_ESCAPE_CHAR )
        {
            handle->rx.escape = 1;
        }
        else if ( handle->rx.data - (uint8_t *)handle->rx_buf < handle->rx_buf_size )
        {
            if ( handle->rx.escape )
            {
                *handle->rx.data = byte ^ TINY_ESCAPE_BIT;
                handle->rx.escape = 0;
            }
            else
            {
                *handle->rx.data = byte;
            }
            handle->rx.data++;
            // LOG(TINY_LOG_DEB, "%02X\n", handle->rx.data[ handle->rx.len ]);
        }
        result++;
        data++;
        len--;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////

static int hdlc_ll_read_end( hdlc_ll_handle_t handle, const uint8_t *data, int len_bytes )
{
    if ( handle->rx.data == handle->rx_buf )
    {
        // Impossible, maybe frame alignment is wrong, go to read data again
        LOG( TINY_LOG_WRN, "[HDLC:%p] RX: error in frame alignment, recovering...\n", handle);
        handle->rx.escape = 0;
        handle->rx.state = hdlc_ll_read_data;
        return 0; // That's OK, we actually didn't process anything from user bytes
    }
    handle->rx.state = hdlc_ll_read_start;
    int len = (int)(handle->rx.data - (uint8_t *)handle->rx_buf);
    if ( len > handle->rx_buf_size )
    {
        // Buffer size issue, too long packet
        LOG( TINY_LOG_ERR, "[HDLC:%p] RX: tool long frame\n", handle);
        return TINY_ERR_DATA_TOO_LARGE;
    }
    if ( len < (uint8_t)handle->crc_type / 8 )
    {
        // CRC size issue
        LOG( TINY_LOG_ERR, "[HDLC:%p] RX: crc field is too short\n", handle);
        return TINY_ERR_WRONG_CRC;
    }
    crc_t calc_crc = 0;
    crc_t read_crc = 0;
    switch ( handle->crc_type )
    {
#ifdef CONFIG_ENABLE_CHECKSUM
        case HDLC_CRC_8:
            calc_crc = chksum( INITCHECKSUM, handle->rx_buf, len - 1 ) & 0x00FF;
            read_crc = handle->rx.data[-1];
            break;
#endif
#ifdef CONFIG_ENABLE_FCS16
        case HDLC_CRC_16:
            calc_crc = crc16( PPPINITFCS16, handle->rx_buf, len - 2 );
            read_crc = handle->rx.data[ -2 ] |
                       ((uint16_t)handle->rx.data[ -1 ] << 8 );
            break;
#endif
#ifdef CONFIG_ENABLE_FCS32
        case HDLC_CRC_32:
            calc_crc = crc32( PPPINITFCS32, handle->rx_buf, len - 4 );
            read_crc = handle->rx.data[ -4 ] |
                       ((uint32_t)handle->rx.data[ -3 ] << 8 ) |
                       ((uint32_t)handle->rx.data[ -2 ] << 16 ) |
                       ((uint32_t)handle->rx.data[ -1 ] << 24 );
            break;
#endif
        default: break;
    }
    if ( calc_crc != read_crc )
    {
        // CRC calculate issue
        #if TINY_HDLC_DEBUG
        LOG( TINY_LOG_ERR, "[HDLC:%p] RX: WRONG CRC (calc:%08X != %08X)\n", handle, calc_crc, read_crc);
        if (TINY_LOG_DEB < g_tiny_log_level) for (int i=0; i< len; i++) fprintf(stderr, " %c ", (char)((uint8_t *)handle->rx_buf)[i]);
        LOG( TINY_LOG_DEB, "\n");
        if (TINY_LOG_DEB < g_tiny_log_level) for (int i=0; i< len; i++) fprintf(stderr, " %02X ", ((uint8_t *)handle->rx_buf)[i]);
        LOG( TINY_LOG_DEB, "\n-----------\n");
        #endif
        return TINY_ERR_WRONG_CRC;
    }
    len -= (uint8_t)handle->crc_type / 8;
    // Shift back data pointer, pointing to the last byte after payload
    handle->rx.data -= (uint8_t)handle->crc_type / 8;
    LOG( TINY_LOG_INFO, "[HDLC:%p] RX: Frame success: %d bytes\n", handle, len);
    if ( handle->on_frame_read )
    {
        handle->on_frame_read( handle->user_data, handle->rx_buf, len );
    }
    return TINY_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////

int hdlc_ll_run_rx( hdlc_ll_handle_t handle, const void *data, int len, int *error )
{
    int result = 0;
    if ( error )
    {
        *error = TINY_SUCCESS;
    }
    while ( len || handle->rx.state == hdlc_ll_read_end )
    {
        int temp_result = handle->rx.state( handle, (const uint8_t *)data, len );
        if ( temp_result <=0 )
        {
            if ( error )
            {
                *error = temp_result;
            }
            break;
        }
        data=(uint8_t *)data + temp_result;
        len -= temp_result;
        result += temp_result;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////

int hdlc_ll_get_buf_size( int mtu )
{
    return get_crc_field_size( HDLC_CRC_32 ) +
           sizeof(hdlc_ll_data_t) +
           mtu;
}

////////////////////////////////////////////////////////////////////////////////////////////

int hdlc_ll_get_buf_size_ex( int mtu, hdlc_crc_t crc_type )
{
    return get_crc_field_size( crc_type ) +
           sizeof(hdlc_ll_data_t) +
           mtu;
}

////////////////////////////////////////////////////////////////////////////////////////////
