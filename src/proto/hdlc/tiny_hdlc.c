#include "tiny_hdlc.h"
#include "proto/crc/crc.h"

#include <stddef.h>

#ifndef TINY_HDLC_DEBUG
#define TINY_HDLC_DEBUG 0
#endif

#if TINY_HDLC_DEBUG
#include <stdio.h>
#define LOG(...)  fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

#define FLAG_SEQUENCE            0x7E
#define TINY_ESCAPE_CHAR         0x7D
#define TINY_ESCAPE_BIT          0x20

enum
{
    TX_ACCEPT_BIT = 0x01,
    TX_DATA_READY_BIT = 0x02,
    TX_DATA_SENT_BIT = 0x04,
    RX_DATA_READY_BIT = 0x08,
};

static int hdlc_read_start( hdlc_handle_t handle, const uint8_t *data, int len );
static int hdlc_read_data( hdlc_handle_t handle, const uint8_t *data, int len );
static int hdlc_read_end( hdlc_handle_t handle, const uint8_t *data, int len );

static int hdlc_send_start( hdlc_handle_t handle );
static int hdlc_send_data( hdlc_handle_t handle );
static int hdlc_send_crc( hdlc_handle_t handle );
static int hdlc_send_end( hdlc_handle_t handle );

hdlc_handle_t hdlc_init( hdlc_struct_t *hdlc_info )
{
    if ( hdlc_info->crc_type == HDLC_CRC_DEFAULT )
    {
#if defined(CONFIG_ENABLE_FCS16)
        hdlc_info->crc_type = HDLC_CRC_16;
#elif defined(CONFIG_ENABLE_FCS32)
        hdlc_info->crc_type = HDLC_CRC_32;
#elif defined(CONFIG_ENABLE_FCS8)
        hdlc_info->crc_type = HDLC_CRC_8;
#endif
    }
    else if ( hdlc_info->crc_type == HDLC_CRC_OFF )
    {
        hdlc_info->crc_type = 0;
    }
    tiny_mutex_create( &hdlc_info->send_mutex );
    tiny_events_create( &hdlc_info->events );
    tiny_events_set( &hdlc_info->events, TX_ACCEPT_BIT );
    // Must be last
    hdlc_reset( hdlc_info );
    return hdlc_info;
}

int hdlc_close( hdlc_handle_t handle )
{
    if ( handle->tx.data )
    {
        if ( handle->on_frame_sent )
        {
            handle->on_frame_sent( handle->user_data, handle->tx.origin_data,
                                   handle->tx.data - handle->tx.origin_data );
        }
    }
    tiny_events_destroy( &handle->events );
    tiny_mutex_destroy( &handle->send_mutex );
    return 0;
}

void hdlc_reset( hdlc_handle_t handle )
{
    handle->rx.state = hdlc_read_start;
    handle->tx.data = NULL;
    handle->tx.state = hdlc_send_start;
    tiny_events_clear( &handle->events, EVENT_BIS_ALL );
    tiny_events_set( &handle->events, TX_ACCEPT_BIT );
}

static int hdlc_send_start( hdlc_handle_t handle )
{
    int bits = tiny_events_wait( &handle->events, TX_DATA_READY_BIT, EVENT_BITS_LEAVE, 0 );
    if ( bits == 0 )
    {
        return 0;
    }
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
    int result = handle->send_tx( handle->user_data, buf, sizeof(buf) );
    if ( result == 1 )
    {
        handle->tx.state = hdlc_send_data;
        handle->tx.escape = 0;
    }
    return result;
}

static int hdlc_send_data( hdlc_handle_t handle )
{
    if ( handle->tx.len == 0 )
    {
        handle->tx.state = hdlc_send_crc;
        return 0;
    }
    int pos = 0;
    while ( handle->tx.data[pos] != FLAG_SEQUENCE &&
            handle->tx.data[pos] != TINY_ESCAPE_CHAR &&
            pos < handle->tx.len )
    {
        pos++;
    }
    int result;
    if ( pos )
    {
        result = handle->send_tx( handle->user_data, handle->tx.data, pos );
        if ( result > 0 )
        {
            handle->tx.data += result;
            handle->tx.len -= result;
        }
    }
    else
    {
        uint8_t buf[1] = { handle->tx.escape ? ( handle->tx.data[0] ^ TINY_ESCAPE_BIT )
                                             : TINY_ESCAPE_CHAR };
        result = handle->send_tx( handle->user_data, buf, sizeof(buf) );
        if ( result == 1 )
        {
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
        handle->tx.state = hdlc_send_crc;
    }
    return result;
}

static int hdlc_send_crc( hdlc_handle_t handle )
{
    int result;
    if ( handle->tx.len == (uint8_t)handle->crc_type )
    {
        handle->tx.state = hdlc_send_end;
        return 1;
    }
    uint8_t byte = handle->tx.crc >> handle->tx.len;
    if ( byte != TINY_ESCAPE_CHAR && byte != FLAG_SEQUENCE )
    {
        result = handle->send_tx( handle->user_data, &byte, sizeof(byte) );
        if ( result == 1 )
        {
            handle->tx.len += 8;
        }
    }
    else
    {
        byte = handle->tx.escape ? ( byte ^ TINY_ESCAPE_BIT ) : TINY_ESCAPE_CHAR;
        result = handle->send_tx( handle->user_data, &byte, sizeof(byte) );
        if ( result == 1 )
        {
            handle->tx.escape = !handle->tx.escape;
            if ( !handle->tx.escape )
            {
                handle->tx.len += 8;
            }
        }
    }
    return result;
}

static int hdlc_send_end( hdlc_handle_t handle )
{
    uint8_t buf[1] = { FLAG_SEQUENCE };
    int result = handle->send_tx( handle->user_data, buf, sizeof(buf) );
    if ( result == 1 )
    {
        tiny_events_clear( &handle->events, TX_DATA_READY_BIT );
        handle->tx.state = hdlc_send_start;
        handle->tx.escape = 0;
        if ( handle->on_frame_sent )
        {
            handle->on_frame_sent( handle->user_data, handle->tx.origin_data,
                                   handle->tx.data - handle->tx.origin_data );
        }
        tiny_events_set( &handle->events, TX_DATA_SENT_BIT );
        tiny_events_set( &handle->events, TX_ACCEPT_BIT );
    }
    return result;
}

int hdlc_run_tx( hdlc_handle_t handle )
{
    int result = 0;
    while (1)
    {
        int temp_result = handle->tx.state( handle );
        if ( temp_result <=0 )
        {
            result = result ? result: temp_result;
            break;
        }
        result += temp_result;
    }
    return result;
}

static int hdlc_run_tx_until_sent( hdlc_handle_t handle, uint32_t timeout )
{
    uint32_t ts = tiny_millis();
    int result = 0;
    for(;;)
    {
        result = handle->tx.state( handle );

        if ( result < 0 )
            break;
        uint8_t bits = tiny_events_wait( &handle->events, TX_DATA_SENT_BIT, EVENT_BITS_CLEAR, 0 );
        if  ( bits != 0 )
        {
            result = TINY_SUCCESS;
            break;
        }
        if ( (uint32_t)(tiny_millis() - ts) >= timeout && timeout != 0xFFFFFFFF )
        {
            result = TINY_ERR_TIMEOUT;
            break;
        }
    };
    return result;
}

static int hdlc_put( hdlc_handle_t handle, const void *data, int len, uint32_t timeout )
{
    // Check if TX thread is ready to accept new data
    if ( tiny_events_wait( &handle->events, TX_ACCEPT_BIT, EVENT_BITS_CLEAR, timeout ) == 0 )
    {
        return TINY_ERR_TIMEOUT;
    }
    handle->tx.origin_data = data;
    handle->tx.data = data;
    handle->tx.len = len;
    // Indicate that now we have something to send
    tiny_events_set( &handle->events, TX_DATA_READY_BIT );
    return TINY_SUCCESS;
}

int hdlc_send( hdlc_handle_t handle, const void *data, int len, uint32_t timeout )
{
    int result = TINY_SUCCESS;
    tiny_mutex_lock( &handle->send_mutex );
    if ( data != NULL )
    {
        result = hdlc_put( handle, data, len, timeout );
        if ( result == TINY_ERR_TIMEOUT ) result = TINY_ERR_BUSY;
    }
    if ( result >= 0 && timeout)
    {
        if ( handle->multithread_mode )
        {
            // in multithreaded mode we must wait, until Tx thread sends the data
            uint8_t bits = tiny_events_wait( &handle->events, TX_DATA_SENT_BIT, EVENT_BITS_CLEAR, timeout );
            result = bits == 0 ? TINY_ERR_TIMEOUT: TINY_SUCCESS;
        }
        else
        {
            // while in single thread mode we must send the data by ourselves
            result = hdlc_run_tx_until_sent( handle, timeout );
        }
    }
    tiny_mutex_unlock( &handle->send_mutex );
    return result;
}

static int hdlc_read_start( hdlc_handle_t handle, const uint8_t *data, int len )
{
    if ( !len )
    {
        return 0;
    }
    if ( data[0] != FLAG_SEQUENCE )
    {
        // TODO: Skip byte, but we received some wrong data
        return 1;
    }
    handle->rx.escape = 0;
    handle->rx.data = (uint8_t *)handle->rx_buf;
    handle->rx.state = hdlc_read_data;
    return 1;
}

static int hdlc_read_data( hdlc_handle_t handle, const uint8_t *data, int len )
{
    if (!len)
    {
        return 0;
    }
    uint8_t byte = data[0];
    if ( byte == FLAG_SEQUENCE )
    {
        handle->rx.state = hdlc_read_end;
        // Leave this byte for CRC check
        return 0;
    }
    if ( byte == TINY_ESCAPE_CHAR )
    {
        handle->rx.escape = 1;
        return 1;
    }
    if ( handle->rx.data - (uint8_t *)handle->rx_buf < handle->rx_buf_size )
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
    }
//    LOG("%02X\n", handle->rx.data[ handle->rx.len ]);
    handle->rx.data++;
    return 1;
}

static int hdlc_read_end( hdlc_handle_t handle, const uint8_t *data, int len_bytes )
{
    if ( handle->rx.data == handle->rx_buf )
    {
        // Impossible, maybe frame alignment is wrong, go to read data again
        LOG( "[HDLC:%p] RX: error in frame alignment, recovering...\n", handle);
        handle->rx.escape = 0;
        handle->rx.state = hdlc_read_data;
        return 1;
    }
    handle->rx.state = hdlc_read_start;
    int len = handle->rx.data - (uint8_t *)handle->rx_buf;
    if ( len > handle->rx_buf_size )
    {
        // Buffer size issue, too long packet
        LOG( "[HDLC:%p] RX: tool long frame\n", handle);
        return 1;
    }
    if ( len < (uint8_t)handle->crc_type / 8 )
    {
        // CRC size issue
        LOG( "[HDLC:%p] RX: crc field is too short\n", handle);
        return 1;
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
        LOG( "[HDLC:%p] RX: WRONG CRC (calc:%08X != %08X)\n", handle, calc_crc, read_crc);
        for (int i=0; i< len; i++) LOG(" %c ", (char)handle->rx_buf[i]);
        LOG("\n");
        for (int i=0; i< len; i++) LOG( " %02X ", handle->rx_buf[i]);
        LOG("\n-----------\n");
        #endif
        return 1;
    }
    // LOG( "[HDLC:%p] RX: GOOD\n", handle);
    len -= (uint8_t)handle->crc_type / 8;
    if ( handle->on_frame_read )
    {
        handle->on_frame_read( handle->user_data, handle->rx_buf, len );
    }
    // Set bit indicating that we have read and processed the frame
    tiny_events_set( &handle->events, RX_DATA_READY_BIT );
    return 1;
}

int hdlc_run_rx( hdlc_handle_t handle, const void *data, int len, int *error )
{
    int result = 0;
    for (;;)
    {
        int temp_result = handle->rx.state( handle, (const uint8_t *)data, len );
        if ( temp_result <=0 )
        {
            if ( error )
            {
                *error = temp_result;
            }
            // Just clear this bit, since the caller doesn't want to wait for any frame
            // This bit is used only by hdlc_run_rx_until_read()
            tiny_events_clear( &handle->events, RX_DATA_READY_BIT );
            break;
        }
        data=(uint8_t *)data + temp_result;
        len -= temp_result;
        result += temp_result;
    }
    return result;
}

void hdlc_set_rx_buffer( hdlc_handle_t handle, void *data, int size)
{
    handle->rx_buf = data;
    handle->rx_buf_size = size;
}

int hdlc_run_rx_until_read( hdlc_handle_t handle, read_block_cb_t readcb, void *user_data, uint16_t timeout )
{
    uint16_t ts = tiny_millis();
    int result = 0;
    for(;;)
    {
        // Read single byte and process it
        uint8_t data;
        result = readcb( user_data, &data, sizeof(data) );
        if ( result > 0 )
        {
            do
            {
                int temp = handle->rx.state( handle, &data, result );
                if ( temp > 0 ) result -= temp;
            } while (result > 0);
        }
        if ( result < 0 )
        {
            break;
        }
        // Check for RX_DATA_READY_BIT without timeout. If frame is ready - exit
        uint8_t bits = tiny_events_wait( &handle->events, RX_DATA_READY_BIT, EVENT_BITS_CLEAR, 0 );
        if ( bits )
        {
            result = handle->rx.data - (uint8_t *)handle->rx_buf;
            break;
        }
        // Check if we need to exit on timeout
        if ( (uint16_t)(tiny_millis() - ts) >= timeout )
        {
            result = TINY_ERR_TIMEOUT;
            break;
        }
    };
    return result;
}
