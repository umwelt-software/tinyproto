#include "tiny_hdlc.h"
#include "crc.h"

#include <stdio.h>

#define FLAG_SEQUENCE            0x7E
#define TINY_ESCAPE_CHAR         0x7D
#define TINY_ESCAPE_BIT          0x20

int hdlc_on_rx_data( hdlc_handle_t handle, void *data, int len )
{
    return 0;
}

static int hdlc_send_start( hdlc_handle_t handle );
static int hdlc_send_data( hdlc_handle_t handle );
static int hdlc_send_crc( hdlc_handle_t handle );
static int hdlc_send_end( hdlc_handle_t handle );

int hdlc_init( hdlc_handle_t handle,
               int (*send_tx)(void *user_data, const void *data, int len),
               void *user_data )
{
    handle->send_tx = send_tx;
    handle->user_data = user_data;
    handle->tx.data = NULL;
    handle->tx.state = hdlc_send_start;
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

int hdlc_send_crc( hdlc_handle_t handle )
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

int hdlc_send_end( hdlc_handle_t handle )
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

