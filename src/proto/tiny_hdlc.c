#include "tiny_hdlc.h"
#include "crc.h"

#include <stdio.h>

#define FLAG_SEQUENCE            0x7E
#define TINY_ESCAPE_CHAR         0x7D
#define TINY_ESCAPE_BIT          0x20

static int hdlc_read_start( hdlc_handle_t handle, uint8_t *data, int len );
static int hdlc_read_data( hdlc_handle_t handle, uint8_t *data, int len );
static int hdlc_read_end( hdlc_handle_t handle, uint8_t *data, int len );

static int hdlc_send_start( hdlc_handle_t handle );
static int hdlc_send_data( hdlc_handle_t handle );
static int hdlc_send_crc( hdlc_handle_t handle );
static int hdlc_send_end( hdlc_handle_t handle );

hdlc_handle_t hdlc_init( hdlc_struct_t *hdlc_info )
{
    hdlc_info->rx.data = (uint8_t *)hdlc_info->rx_buf;
    hdlc_info->rx.state = hdlc_read_start;
    hdlc_info->tx.data = NULL;
    hdlc_info->tx.state = hdlc_send_start;
    return hdlc_info;
}

int hdlc_close( hdlc_handle_t handle )
{
    return 0;
}

static int hdlc_send_start( hdlc_handle_t handle )
{
    if ( handle->tx.data == NULL )
    {
        return 0;
    }
    handle->tx.crc = crc16( PPPINITFCS16, handle->tx.data, handle->tx.len );
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
        handle->tx.data = NULL;
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
        if ( handle->tx.len == 0 )
        {
            handle->tx.data = NULL;
            handle->tx.state = hdlc_send_crc;
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
            if ( handle->tx.len == 0 )
            {
                handle->tx.data = NULL;
                handle->tx.state = hdlc_send_crc;
            }
        }
    }
    return result;
}

static int hdlc_send_crc( hdlc_handle_t handle )
{
    int result;
    uint8_t byte = handle->tx.crc >> (8 * handle->tx.len);
    if ( byte != TINY_ESCAPE_CHAR && byte != FLAG_SEQUENCE )
    {
        result = handle->send_tx( handle->user_data, &byte, sizeof(byte) );
        if ( result == 1 )
        {
            handle->tx.len++;
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
                handle->tx.len++;
            }
        }
    }
    if ( handle->tx.len == 2 )
    {
        handle->tx.state = hdlc_send_end;
    }
    return result;
}

static int hdlc_send_end( hdlc_handle_t handle )
{
    uint8_t buf[1] = { FLAG_SEQUENCE };
    int result = handle->send_tx( handle->user_data, buf, sizeof(buf) );
    if ( result == 1 )
    {
        handle->tx.data = NULL;
        handle->tx.state = hdlc_send_start;
        handle->tx.escape = 0;
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
            result = result ?: temp_result;
            break;
        }
        result += temp_result;
    }
    return result;
}

int hdlc_send( hdlc_handle_t handle, void *data, int len )
{
    if ( handle->tx.data != NULL )
    {
        return 0;
    }
    handle->tx.data = data;
    handle->tx.len = len;
    return 1;
}


static int hdlc_read_start( hdlc_handle_t handle, uint8_t *data, int len )
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
    handle->rx.state = hdlc_read_data;
    return 1;
}

static int hdlc_read_data( hdlc_handle_t handle, uint8_t *data, int len )
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
    if ( handle->rx.escape )
    {
        handle->rx.data[ handle->rx.len ] = byte ^ TINY_ESCAPE_BIT;
        handle->rx.escape = 0;
    }
    else
    {
        handle->rx.data[ handle->rx.len ] = byte;
    }
//    printf("%02X\n", handle->rx.data[ handle->rx.len ]);
    handle->rx.len++;
    return 1;
}

static int hdlc_read_end( hdlc_handle_t handle, uint8_t *data, int len )
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
    if ( handle->rx.len < 2 )
    {
        // CRC size issue
        return 0;
    }
    uint16_t calc_crc = crc16( PPPINITFCS16, handle->rx.data, handle->rx.len - 2 );
    uint16_t read_crc = handle->rx.data[ handle->rx.len - 2 ] |
                        ((uint16_t)handle->rx.data[ handle->rx.len - 1 ] << 8 );
    if ( calc_crc != read_crc )
    {
        // CRC calculate issue
        return 0;
    }
    if ( handle->on_frame_data )
    {
        handle->on_frame_data( handle->user_data, handle->rx.data, handle->rx.len - 2 );
    }
    return 0;
}

int hdlc_on_rx_data( hdlc_handle_t handle, void *data, int len )
{
    int result = 0;
    for (;;)
    {
        int temp_result = handle->rx.state( handle, (uint8_t *)data, len );
        if ( temp_result <=0 )
        {
            result = result ?: temp_result;
            break;
        }
        data=(uint8_t *)data + temp_result;
        len -= temp_result;
        result += temp_result;
    }
    return result;
}
