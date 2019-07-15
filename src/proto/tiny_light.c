/*
    Copyright 2017 (C) Alexey Dynda

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
    return TINY_NO_ERROR;
}
#endif

/**************************************************************
*
*                 OPEN/CLOSE FUNCTIONS
*
***************************************************************/

int tiny_light_init(void *handle,
                    write_block_cb_t write_func,
                    read_block_cb_t read_func,
                    void *pdata)
{
    if (!handle || !write_func || !read_func)
    {
        return TINY_ERR_FAILED;
    }
    ((STinyLightData *)handle)->hdlc.user_data = pdata;
    ((STinyLightData *)handle)->hdlc.send_tx = write_func;
    ((STinyLightData *)handle)->hdlc.rx_buf = NULL;
    ((STinyLightData *)handle)->hdlc.rx_buf_size = 0;
    ((STinyLightData *)handle)->hdlc.crc_type = HDLC_CRC_OFF;
    ((STinyLightData *)handle)->read_func = read_func;

#ifdef CONFIG_ENABLE_STATS
    tiny_light_clear_stat((STinyLightData *)handle);
#endif
    hdlc_init( &((STinyLightData *)handle)->hdlc );
    return TINY_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////

int tiny_light_close(void *handle)
{
    if (!handle)
    {
        return TINY_ERR_FAILED;
    }
    ((STinyLightData *)handle)->read_func = 0;
    hdlc_close( &((STinyLightData *)handle)->hdlc );
    return TINY_NO_ERROR;
}

/**************************************************************
*
*                 SEND FUNCTIONS
*
***************************************************************/

/**
 * Returns negative value in case of error
 *         length of sent data
 */
int tiny_light_send(void *handle, const uint8_t * pbuf, int len)
{
    if ( hdlc_send( &((STinyLightData *)handle)->hdlc, pbuf, len ) == 0 )
    {
        return TINY_ERR_FAILED;
    }
    while ( ((STinyLightData *)handle)->hdlc.tx.data )
    {
        int result = hdlc_run_tx( &((STinyLightData *)handle)->hdlc );
        if ( result < 0 )
        {
            return TINY_ERR_FAILED;
        }
    }
    return len;
}

//////////////////////////////////////////////////////////////////////////////////////


/**************************************************************
*
*                 RECEIVE FUNCTIONS
*
***************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////

int tiny_light_read(void *handle, uint8_t *pbuf, int len)
{
    ((STinyLightData *)handle)->hdlc.rx_buf = pbuf;
    ((STinyLightData *)handle)->hdlc.rx.data = pbuf;
    ((STinyLightData *)handle)->hdlc.rx_buf_size = len;
    for(;;)
    {
        uint8_t byte;
        int result = ((STinyLightData *)handle)->read_func(((STinyLightData *)handle)->hdlc.user_data, &byte, 1);
        if ( result <= 0 )
        {
            return TINY_ERR_FAILED;
        }
        hdlc_run_rx( &((STinyLightData *)handle)->hdlc, &byte, 1 );
        if (1)
        {
            // TODO: Add mechanism of checking that packet is received
            break;
        }
    }
    return len;
}

/////////////////////////////////////////////////////////////////////////////////////////

