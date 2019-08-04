#include "tiny_hdlc.h"
#include "proto/crc/crc.h"

#include <stdio.h>

#define FLAG_SEQUENCE            0x7E
#define TINY_ESCAPE_CHAR         0x7D
#define TINY_ESCAPE_BIT          0x20

static int hdlc_read_start( hdlc_handle_t handle, const uint8_t *data, int len );
static int hdlc_read_data( hdlc_handle_t handle, const uint8_t *data, int len );
static int hdlc_read_end( hdlc_handle_t handle, const uint8_t *data, int len );

static int hdlc_send_start( hdlc_handle_t handle );
static int hdlc_send_data( hdlc_handle_t handle );
static int hdlc_send_crc( hdlc_handle_t handle );
static int hdlc_send_end( hdlc_handle_t handle );

hdlc_handle_t hdlc_init( hdlc_struct_t *hdlc_info )
{
    hdlc_reset( hdlc_info );
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
    tiny_mutex_create( &hdlc_info->mutex );
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
    tiny_mutex_destroy( &handle->mutex );
    return 0;
}

void hdlc_reset( hdlc_handle_t handle )
{
    tiny_mutex_lock( &handle->mutex );
    handle->rx.state = hdlc_read_start;
    handle->tx.data = NULL;
    handle->tx.state = hdlc_send_start;
    tiny_mutex_unlock( &handle->mutex );
}

static int hdlc_send_start( hdlc_handle_t handle )
{
    tiny_mutex_lock( &handle->mutex );
    if ( handle->tx.data == NULL )
    {
        tiny_mutex_unlock( &handle->mutex );
        return 0;
    }
    // No need to lock mutex forever until tx.data contains some value
    tiny_mutex_unlock( &handle->mutex );
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
        const uint8_t *data = handle->tx.data;
        tiny_mutex_lock( &handle->mutex );
        handle->tx.data = NULL;
        handle->tx.state = hdlc_send_start;
        handle->tx.escape = 0;
        tiny_mutex_unlock( &handle->mutex );
        if ( handle->on_frame_sent )
        {
            handle->on_frame_sent( handle->user_data, handle->tx.origin_data,
                                   data - handle->tx.origin_data );
        }
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

int hdlc_run_tx_until_sent( hdlc_handle_t handle )
{
    int result = 0;
    do
    {
        int temp_result = handle->tx.state( handle );
        if ( temp_result < 0 )
        {
            result = result ? result: temp_result;
            break;
        }
        result += temp_result;
    } while ( handle->tx.state != hdlc_send_start );
    return result;
}

int hdlc_put( hdlc_handle_t handle, const void *data, int len )
{
    tiny_mutex_lock( &handle->mutex );
    if ( handle->tx.data != NULL )
    {
        tiny_mutex_unlock( &handle->mutex );
        return 0;
    }
    handle->tx.origin_data = data;
    handle->tx.data = data;
    handle->tx.len = len;
    tiny_mutex_unlock( &handle->mutex );
    return 1;
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
    handle->rx.len = 0;
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
        return 1;
    }
    if ( byte == TINY_ESCAPE_CHAR )
    {
        handle->rx.escape = 1;
        return 1;
    }
    if ( handle->rx.len < handle->rx_buf_size )
    {
        if ( handle->rx.escape )
        {
            handle->rx.data[ handle->rx.len ] = byte ^ TINY_ESCAPE_BIT;
            handle->rx.escape = 0;
        }
        else
        {
            handle->rx.data[ handle->rx.len ] = byte;
        }
    }
//    printf("%02X\n", handle->rx.data[ handle->rx.len ]);
    handle->rx.len++;
    return 1;
}

static int hdlc_read_end( hdlc_handle_t handle, const uint8_t *data, int len )
{
    if ( handle->rx.len == 0)
    {
        // Impossible, maybe frame alignment is wrong, go to read data again
        handle->rx.len = 0;
        handle->rx.escape = 0;
        handle->rx.state = hdlc_read_data;
        return 0;
    }
    handle->rx.state = hdlc_read_start;
    if ( handle->rx.len > handle->rx_buf_size )
    {
        // Buffer size issue, too long packet
        return 0;
    }
    if ( handle->rx.len < (uint8_t)handle->crc_type / 8 )
    {
        // CRC size issue
        return 0;
    }
    crc_t calc_crc = 0;
    crc_t read_crc = 0;
    switch ( handle->crc_type )
    {
#ifdef CONFIG_ENABLE_CHECKSUM
        case HDLC_CRC_8:
            calc_crc = chksum( INITCHECKSUM, handle->tx.data, handle->tx.len - 1 );
            read_crc = handle->rx.data[ handle->rx.len - 1 ];
            break;
#endif
#ifdef CONFIG_ENABLE_FCS16
        case HDLC_CRC_16:
            calc_crc = crc16( PPPINITFCS16, handle->rx.data, handle->rx.len - 2 );
            read_crc = handle->rx.data[ handle->rx.len - 2 ] |
                       ((uint16_t)handle->rx.data[ handle->rx.len - 1 ] << 8 );
            break;
#endif
#ifdef CONFIG_ENABLE_FCS32
        case HDLC_CRC_32:
            calc_crc = crc32( PPPINITFCS32, handle->rx.data, handle->rx.len - 4 );
            read_crc = handle->rx.data[ handle->rx.len - 4 ] |
                       ((uint32_t)handle->rx.data[ handle->rx.len - 3 ] << 8 ) |
                       ((uint32_t)handle->rx.data[ handle->rx.len - 2 ] << 16 ) |
                       ((uint32_t)handle->rx.data[ handle->rx.len - 1 ] << 24 );
            break;
#endif
        default: break;
    }
    if ( calc_crc != read_crc )
    {
        // CRC calculate issue
        return 0;
    }
    if ( handle->on_frame_read )
    {
        handle->on_frame_read( handle->user_data, handle->rx.data, handle->rx.len - (uint8_t)handle->crc_type / 8 );
    }
    return 0;
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
    handle->rx.data = (uint8_t *)handle->rx_buf;
}

int hdlc_run_rx_until_read( hdlc_handle_t handle, read_block_cb_t readcb, int *error )
{
    int result = 0;
    do
    {
        uint8_t data;
        int temp_result = readcb( handle->user_data, &data, sizeof(data) );
        if ( temp_result > 0 )
        {
            temp_result = handle->rx.state( handle, &data, temp_result );
        }
        if ( temp_result < 0 )
        {
            if ( error )
            {
                *error = temp_result;
            }
            break;
        }
        result += temp_result;
    } while ( handle->rx.state != hdlc_read_start );
    return result;
}
