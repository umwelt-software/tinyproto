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
    ((STinyLightData *)handle)->pdata = pdata;
    ((STinyLightData *)handle)->write_func = write_func;
    ((STinyLightData *)handle)->read_func = read_func;
#ifdef CONFIG_ENABLE_STATS
    tiny_clear_stat(handle);
#endif
    return TINY_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////

int tiny_light_close(void *handle)
{
    if (!handle)
    {
        return TINY_ERR_FAILED;
    }
    ((STinyLightData *)handle)->write_func = 0;
    ((STinyLightData *)handle)->read_func = 0;
    return TINY_NO_ERROR;
}

/**************************************************************
*
*                 SEND FUNCTIONS
*
***************************************************************/

//////////////////////////////////////////////////////////////////////////////////////

static int tiny_send_byte(void *handle, uint8_t byte)
{
    int result;
    if ( (byte == FLAG_SEQUENCE) || (byte == TINY_ESCAPE_CHAR) )
    {
        uint8_t escape = TINY_ESCAPE_CHAR;
        byte ^= TINY_ESCAPE_BIT;
        result = ((STinyLightData *)handle)->write_func(((STinyLightData *)handle)->pdata, &escape, 1);
        if ( result <= 0 )
        {
            return result;
        }
    }
    result = ((STinyLightData *)handle)->write_func(((STinyLightData *)handle)->pdata, &byte, 1);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns negative value in case of error
 *         length of sent data
 */
int tiny_light_send(void *handle, const uint8_t * pbuf, int len)
{
    int result = TINY_NO_ERROR;

    uint8_t byte = FLAG_SEQUENCE;
    result = ((STinyLightData *)handle)->write_func(((STinyLightData *)handle)->pdata, &byte, 1);
    if (result <= 0)
    {
        return TINY_ERR_FAILED;
    }
    uint8_t length = len;
    while ( length )
    {
        result = tiny_send_byte( handle, *pbuf);
        if (result <= 0)
        {
            return TINY_ERR_FAILED;
        }
        length--;
        pbuf++;
    }
    result = ((STinyLightData *)handle)->write_func(((STinyLightData *)handle)->pdata, &byte, 1);
    if (result <= 0)
    {
        return TINY_ERR_FAILED;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////


/**************************************************************
*
*                 RECEIVE FUNCTIONS
*
***************************************************************/

static int tiny_read_byte(void * handle, uint8_t *byte)
{
    int error;
    error = ((STinyLightData *)handle)->read_func(((STinyLightData *)handle)->pdata, byte, 1);
    if ( error <= 0 )
    {
        return error;
    }
    if (*byte == FLAG_SEQUENCE)
    {
        return FLAG_SEQUENCE;
    }
    if (*byte != TINY_ESCAPE_CHAR)
    {
        return 1;
    }
    error = ((STinyLightData *)handle)->read_func(((STinyLightData *)handle)->pdata, byte, 1);
    if ( error <= 0 )
    {
        return error;
    }
    if (*byte == FLAG_SEQUENCE)
    {
        return FLAG_SEQUENCE;
    }
    *byte ^= TINY_ESCAPE_BIT;
    return error;
}

/////////////////////////////////////////////////////////////////////////////////////////

int tiny_light_read(void *handle, uint8_t *pbuf, int len)
{
    uint8_t length = 0;
    int result = TINY_NO_ERROR;
    if ( len <= 0 )
    {
        return TINY_ERR_FAILED;
    }
    result = ((STinyLightData *)handle)->read_func(((STinyLightData *)handle)->pdata, pbuf, 1);
    if ((1 != result) || (*pbuf != FLAG_SEQUENCE))
    {
        return TINY_ERR_OUT_OF_SYNC;
    }
    while (length < len)
    {
        result = tiny_read_byte( handle, pbuf );
        if (result <= 0)
        {
            result = TINY_ERR_FAILED;
            break;
        }
        if ( FLAG_SEQUENCE == result )
        {
            if (length != 0)
            {
                result = length;
                break;
            }
            continue;
        }
        length++;
        pbuf++;
    }
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

