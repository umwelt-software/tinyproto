/*
    Copyright 2017-2022 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

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

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/

#include "tiny_light.h"
#include "proto/hdlc/low_level/hdlc_int.h"
#include <stddef.h>

#ifdef CONFIG_ENABLE_STATS
#define STATS(x) x
#else
#define STATS(x)
#endif

/************************************************************
 *
 *  INTERNAL FRAME STRUCTURE
 *
 *   8     any len       8
 * | 7E |  USER DATA  | 7E |
 *
 * 7E is standard frame delimiter (commonly use on layer 2)
 * FCS is standard checksum. Refer to RFC1662 for example.
 * UID is reserved for future usage
 *************************************************************/

#define FLAG_SEQUENCE 0x7E
#define TINY_ESCAPE_CHAR 0x7D
#define TINY_ESCAPE_BIT 0x20

#ifdef SOC_UART_FIFO_LEN
#define TINY_LIGHT_STREAM_BUFF_LEN  SOC_UART_FIFO_LEN
#endif

//////////////////////////////////////////////////////////////////////////////

/**************************************************************
 *
 *                 OPEN/CLOSE FUNCTIONS
 *
 ***************************************************************/

static int on_frame_read(void *user_data, void *data, int len);
static int on_frame_sent(void *user_data, const void *data, int len);

int tiny_light_init(STinyLightData *handle, write_block_cb_t write_func, read_block_cb_t read_func, void *pdata)
{
    if ( !handle || !write_func || !read_func )
    {
        return TINY_ERR_FAILED;
    }
    hdlc_ll_init_t init = { 0 };
    init.user_data = handle;
    init.on_frame_read = on_frame_read;
    init.on_frame_sent = on_frame_sent;
    init.buf = &handle->buffer[0];
    init.buf_size = LIGHT_BUF_SIZE;
    init.crc_type = ((STinyLightData *)handle)->crc_type;

    handle->user_data = pdata;
    handle->read_func = read_func;
    handle->write_func = write_func;

    return hdlc_ll_init(&handle->_hdlc, &init);
}

//////////////////////////////////////////////////////////////////////////////

int tiny_light_close(STinyLightData *handle)
{
    if ( !handle )
    {
        return TINY_ERR_FAILED;
    }
    hdlc_ll_close(handle->_hdlc);
    return TINY_SUCCESS;
}

/**************************************************************
 *
 *                 SEND FUNCTIONS
 *
 ***************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////

static int on_frame_sent(void *user_data, const void *data, int len)
{
    return len;
}

/////////////////////////////////////////////////////////////////////////////////////////

int tiny_light_send(STinyLightData *handle, const uint8_t *pbuf, int len)
{
    uint32_t ts = tiny_millis();
    int result = TINY_SUCCESS;
    hdlc_ll_put(handle->_hdlc, pbuf, len);
    while ( handle->_hdlc->tx.origin_data )
    {
#ifdef TINY_LIGHT_STREAM_BUFF_LEN
        uint8_t stream[TINY_LIGHT_STREAM_BUFF_LEN];
#else
        uint8_t stream[1];
#endif
        int stream_len = hdlc_ll_run_tx(handle->_hdlc, stream, sizeof(stream));
        do
        {
            result = handle->write_func(handle->user_data, stream, stream_len);
            if ( result < 0 )
            {
                return result;
            }
            if ( (uint32_t)(tiny_millis() - ts) >= 1000 )
            {
                hdlc_ll_reset(handle->_hdlc, HDLC_LL_RESET_TX_ONLY);
                result = TINY_ERR_TIMEOUT;
                break;
            }
        } while ( result < stream_len );
    }
    return result >= 0 ? len : result;
}

//////////////////////////////////////////////////////////////////////////////////////

/**************************************************************
 *
 *                 RECEIVE FUNCTIONS
 *
 ***************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////

static int on_frame_read(void *user_data, void *data, int len)
{
    STinyLightData *handle = (STinyLightData *)user_data;
    handle->rx_len = len;
    return len;
}

/////////////////////////////////////////////////////////////////////////////////////////

int tiny_light_read(STinyLightData *handle, uint8_t *pbuf, int len)
{
    uint32_t ts = tiny_millis();
    int result = 0;
    handle->_hdlc->rx_buf = pbuf;
    handle->_hdlc->rx_buf_size = len;
    handle->rx_len = 0;
    do
    {
        uint8_t stream[1];
        int stream_len = handle->read_func(handle->user_data, stream, sizeof(stream));
        if ( stream_len < 0 )
        {
            result = stream_len;
            break;
        }
        hdlc_ll_run_rx(handle->_hdlc, stream, stream_len, &result);
        if ( result == TINY_SUCCESS )
        {
            hdlc_ll_run_rx(handle->_hdlc, stream, 0, &result);
        }
        if ( result != TINY_SUCCESS )
        {
            break;
        }
        if ( (uint32_t)(tiny_millis() - ts) >= 1000 )
        {
            hdlc_ll_reset(handle->_hdlc, HDLC_LL_RESET_RX_ONLY);
            result = TINY_ERR_TIMEOUT;
            break;
        }
    } while ( handle->rx_len == 0 );
    if ( handle->rx_len != 0 )
    {
        result = handle->rx_len;
    }
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

hdlc_ll_handle_t tiny_light_get_hdlc(STinyLightData *handle)
{
    return handle->_hdlc;
}

/////////////////////////////////////////////////////////////////////////////////////////
