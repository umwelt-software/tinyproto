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

#include "tiny_light.h"
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

#define FLAG_SEQUENCE            0x7E
#define TINY_ESCAPE_CHAR         0x7D
#define TINY_ESCAPE_BIT          0x20

//////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_ENABLE_STATS
static int tiny_light_clear_stat(STinyLightData *handle)
{
    if (!handle)
        return TINY_ERR_INVALID_DATA;
    handle->stat.bytesSent = 0;
    handle->stat.bytesReceived = 0;
    handle->stat.framesBroken = 0;
    handle->stat.framesReceived = 0;
    handle->stat.framesSent = 0;
    handle->stat.oosyncBytes = 0;
    return TINY_SUCCESS;
}
#endif

/**************************************************************
*
*                 OPEN/CLOSE FUNCTIONS
*
***************************************************************/

static int write_func_cb(void *user_data, const void *data, int len);
static int on_frame_read(void *user_data, void *data, int len);
static int on_frame_sent(void *user_data, const void *data, int len);

int tiny_light_init(void *handle,
                    write_block_cb_t write_func,
                    read_block_cb_t read_func,
                    void *pdata)
{
    if (!handle || !write_func || !read_func)
    {
        return TINY_ERR_FAILED;
    }
    ((STinyLightData *)handle)->_hdlc.user_data = handle;
    ((STinyLightData *)handle)->_hdlc.on_frame_read = on_frame_read;
    ((STinyLightData *)handle)->_hdlc.on_frame_sent = on_frame_sent;
    ((STinyLightData *)handle)->_hdlc.send_tx = write_func_cb;
    ((STinyLightData *)handle)->_hdlc.rx_buf = NULL;
    ((STinyLightData *)handle)->_hdlc.rx_buf_size = 0;
    ((STinyLightData *)handle)->_hdlc.crc_type = ((STinyLightData *)handle)->crc_type;

    ((STinyLightData *)handle)->user_data = pdata;
    ((STinyLightData *)handle)->read_func = read_func;
    ((STinyLightData *)handle)->write_func = write_func;

#ifdef CONFIG_ENABLE_STATS
    tiny_light_clear_stat((STinyLightData *)handle);
#endif
    hdlc_init( &((STinyLightData *)handle)->_hdlc );
    return TINY_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////

int tiny_light_close(void *handle)
{
    if (!handle)
    {
        return TINY_ERR_FAILED;
    }
    ((STinyLightData *)handle)->read_func = 0;
    hdlc_close( &((STinyLightData *)handle)->_hdlc );
    return TINY_SUCCESS;
}

/**************************************************************
*
*                 SEND FUNCTIONS
*
***************************************************************/

static int write_func_cb(void *user_data, const void *data, int len)
{
    return ((STinyLightData *)user_data)->write_func(
                    ((STinyLightData *)user_data)->user_data,
                    data,
                    len);
}

static int on_frame_sent(void *user_data, const void *data, int len)
{
    return len;
}

int tiny_light_send(void *handle, const uint8_t * pbuf, int len)
{
    int result = hdlc_send( &((STinyLightData *)handle)->_hdlc, pbuf, len, 1000 );
    return result >= 0 ? len: result;
}

//////////////////////////////////////////////////////////////////////////////////////


/**************************************************************
*
*                 RECEIVE FUNCTIONS
*
***************************************************************/

static int on_frame_read(void *user_data, void *data, int len)
{
    return len;
}


int tiny_light_read(void *handle, uint8_t *pbuf, int len)
{
    hdlc_set_rx_buffer( &((STinyLightData *)handle)->_hdlc, pbuf, len);
    int result = hdlc_run_rx_until_read( &((STinyLightData *)handle)->_hdlc,
                                          ((STinyLightData *)handle)->read_func,
                                          ((STinyLightData *)handle)->user_data, 1000 );
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

hdlc_handle_t tiny_light_get_hdlc(void *handle)
{
    return &((STinyLightData *)handle)->_hdlc;
}
