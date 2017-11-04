/*
    Copyright 2016-2017 (C) Alexey Dynda

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

#include "crc.h"
#include "tiny_layer2.h"

#ifdef CONFIG_ENABLE_STATS
    #define STATS(x) x
#else
    #define STATS(x)
#endif

/************************************************************
*
*  INTERNAL FRAME STRUCTURE
*
*   8    16     any len     8/16/32  8
* | 7E | UID |  USER DATA  |  FCS  | 7E |
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
*                 FCS/CHKSUM FUNCTIONS
*
***************************************************************/

inline static void __init_fcs_field(uint8_t fcs_bits, fcs_t* fcs)
{
    switch (fcs_bits)
    {
#ifdef CONFIG_ENABLE_FCS16
    case 16:
        *fcs = PPPINITFCS16; break;
#endif
#ifdef CONFIG_ENABLE_FCS32
    case 32:
        *fcs = PPPINITFCS32; break;
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
    case 8:
        *fcs = INITCHECKSUM; break;
#endif
    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////

inline static int __check_fcs_field(uint8_t fcs_bits, fcs_t fcs)
{
    /* Check CRC */
#ifdef CONFIG_ENABLE_FCS32
    if ((fcs_bits == 32) && ((uint32_t)fcs == PPPGOODFCS32)) return 1;
#endif
#ifdef CONFIG_ENABLE_FCS16
    if ((fcs_bits == 16) && ((uint16_t)fcs == PPPGOODFCS16)) return 1;
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
    if ((fcs_bits == 8) && ((uint8_t)fcs == GOODCHECKSUM)) return 1;
#endif
    return fcs_bits == 0;
}

///////////////////////////////////////////////////////////////////////////////

inline static void  __update_fcs_field(uint8_t fcs_bits, fcs_t* fcs, uint8_t byte)
{
    /* CRC is calculated only over real user data and frame header */
    switch (fcs_bits)
    {
#ifdef CONFIG_ENABLE_FCS16
    case 16:
        *fcs = crc16_byte(*fcs, byte); break;
#endif
#ifdef CONFIG_ENABLE_FCS32
    case 32:
        *fcs = crc32_byte(*fcs, byte); break;
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
    case 8:
        *fcs += byte; break;
#endif
    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////

inline static void  __commit_fcs_field(uint8_t fcs_bits, fcs_t* fcs)
{
    /* CRC is calculated only over real user data and frame header */
    switch (fcs_bits)
    {
#ifdef CONFIG_ENABLE_FCS16
    case 16:
        *fcs = *fcs^0xFFFF; break;
#endif
#ifdef CONFIG_ENABLE_FCS32
    case 32:
        *fcs = *fcs^0xFFFFFFFF; break;
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
    case 8:
        *fcs = 0x00 - *fcs; break;
#endif
    default:
        break;
    }
}

/**************************************************************
*
*                 OPEN/CLOSE FUNCTIONS
*
***************************************************************/


int tiny_init(STinyData *handle,
              write_block_cb_t write_func,
              read_block_cb_t read_func,
              void *pdata)
{
    if (!handle || !write_func)
    {
        return TINY_ERR_INVALID_DATA;
    }
    handle->pdata = pdata;
    handle->write_func = write_func;
    handle->read_func = read_func;
#ifdef CONFIG_ENABLE_STATS
    handle->read_cb = 0;
    handle->write_cb = 0;
#endif
    #if defined(CONFIG_ENABLE_FCS32)
        handle->fcs_bits = 32;
    #elif defined(CONFIG_ENABLE_FCS16)
        handle->fcs_bits = 16;
    #else
        handle->fcs_bits = 8;
    #endif
    handle->rx.inprogress = TINY_RX_STATE_IDLE;
    handle->tx.inprogress = TINY_TX_STATE_IDLE;
    handle->tx.pframe = 0;
    handle->tx.locked = 0;
    MUTEX_INIT(handle->send_mutex);
    COND_INIT(handle->send_condition);
#ifdef CONFIG_ENABLE_STATS
    tiny_clear_stat(handle);
#endif
    return TINY_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////

int tiny_close(STinyData *handle)
{
    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    if ( MUTEX_TRY_LOCK(handle->send_mutex) == 0 ) {};
    MUTEX_UNLOCK(handle->send_mutex);
    handle->tx.inprogress = TINY_TX_STATE_IDLE;
    handle->rx.inprogress = TINY_RX_STATE_IDLE;
    COND_DESTROY(handle->send_condition);
    MUTEX_DESTROY(handle->send_mutex);
    handle->write_func = 0;
    handle->read_func = 0;
    return TINY_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_set_fcs_bits(STinyData *handle, uint8_t bits)
{
    int result = TINY_NO_ERROR;
    switch (bits)
    {
#ifdef CONFIG_ENABLE_FCS16
    case 16:
#endif
#ifdef CONFIG_ENABLE_FCS32
    case 32:
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
    case 8:
#endif
    case 0:
        handle->fcs_bits = bits;
        break;
    default:
        result = TINY_ERR_FAILED;
        break;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

void tiny_enable_uid(STinyData *handle, uint8_t on)
{
    handle->uid_support = on;
}

/**************************************************************
*
*                 SEND FUNCTIONS
*
***************************************************************/

/////////////////////////////////////////////////////////////////////////

/**
 * Returns positive value if flag is sent,
 *         0 if timeout
 *         negative value in case of error
 */
static int __send_frame_flag_state(STinyData *handle)
{
    uint8_t tempbuf[1];
    int result;
    /* Start frame transmission */
    tempbuf[0] = FLAG_SEQUENCE;
    result = handle->write_func(handle->pdata, tempbuf, 1);
    if (result < 0)
    {
        return TINY_ERR_FAILED;
    }
    if (result == 0)
    {
        return TINY_NO_ERROR;
    }
    if (handle->tx.inprogress == TINY_TX_STATE_IDLE)
    {
         handle->tx.sentbytes = 0;
         handle->tx.prevbyte = 0;
         __init_fcs_field(handle->fcs_bits, &handle->tx.fcs);
         handle->tx.inprogress = TINY_TX_STATE_SEND_DATA;
    }
    else
    {
         handle->tx.inprogress = TINY_TX_STATE_IDLE;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////

/**
 * Sends byte to communication channel. Converts byte if escape sequence
 * should be sent.
 * @return TINY_NO_ERROR if escape byte is sent
 *         TINY_ERR_BUSY if byte cannot be sent to the channel right now
 *         TINY_ERR_FAILED if failed to send byte
 *         TINY_SUCCESS if byte is sent
 */
static int __send_byte_state(STinyData *handle, uint8_t byte)
{
    int result;
    /* If byte value equals to some predefined char, send escape char first */
    if (handle->tx.prevbyte == TINY_ESCAPE_CHAR)
    {
        byte ^= TINY_ESCAPE_BIT;
    }
    else if ((byte == TINY_ESCAPE_CHAR) || (byte == FLAG_SEQUENCE))
    {
        byte = TINY_ESCAPE_CHAR;
    }
    result = handle->write_func(handle->pdata, &byte, 1);
    if (result == 0)
    {
        result = TINY_ERR_BUSY;
    }
    if (result < 0)
    {
        return result;
    }
    handle->tx.prevbyte = byte;
    if ( TINY_ESCAPE_CHAR == byte )
    {
        return TINY_NO_ERROR;
    }
    return TINY_SUCCESS;
}

////////////////////////////////////////////////////////////////////////

/**
 * This is handler for SEND DATA state. It switches to
 * TINY_TX_STATE_END only if all bytes are sent.
 * If next byte is successfully sent, it returns 1.
 * If switched to other state, it returns 1.
 * If no byte is sent during the cycle, it returns 0.
 * If error happened, it returns negative value.
 * @param handle - pointer to Tiny Protocol structure
 * @return TINY_NO_ERROR if in progress,
 *         TINY_SUCCESS if buffer is sent completely
 *         TINY_ERR_FAILED if error occured
 *         TINY_ERR_BUSY if data cannot be sent right now
 */
static int __send_frame_data_state(STinyData *handle)
{
    int result;
    uint8_t byte;
    if (handle->tx.sentbytes >= handle->tx.framelen)
    {
        /* sending crc */
        handle->tx.inprogress = handle->fcs_bits ? TINY_TX_STATE_SEND_CRC : TINY_TX_STATE_END;
        handle->tx.bits = (uint8_t)-8;
        return TINY_SUCCESS;
    }
    byte = handle->tx.pframe[handle->tx.sentbytes];
    result = __send_byte_state(handle, byte);
    if ( result == TINY_SUCCESS )
    {
        __update_fcs_field(handle->fcs_bits, &handle->tx.fcs, byte);
        handle->tx.sentbytes++;
        result = TINY_NO_ERROR;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////

static int __send_frame_crc_state(STinyData *handle)
{
    int result;
    uint8_t byte;
    if (handle->tx.bits == (uint8_t)-8)
    {
         handle->tx.bits = 0;
         __commit_fcs_field(handle->fcs_bits, &handle->tx.fcs);
    }
    byte = (handle->tx.fcs >> handle->tx.bits) & 0x000000FF;
    result = __send_byte_state(handle, byte);
    if (result > 0)
    {
        handle->tx.bits += 8;
        if (handle->tx.bits == handle->fcs_bits)
        {
            handle->tx.inprogress = TINY_TX_STATE_END;
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////

int tiny_send_start(STinyData *handle, uint8_t flags)
{
    int result = TINY_ERR_FAILED;

    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }

    result = tiny_lock( handle, flags );
    if (result != TINY_SUCCESS)
    {
        return result;
    }
    if (handle->tx.inprogress != TINY_TX_STATE_IDLE)
    {
        tiny_unlock( handle );
        return TINY_ERR_FAILED;
    }

    do
    {
        result = __send_frame_flag_state(handle);
        /* Exit in case of error or flags is sent */
        if ( result != 0 )
        {
            break;
        }
        TASK_YIELD();
    } while (flags & TINY_FLAG_WAIT_FOREVER);
    /* Failed to send marker for some reason: maybe device is busy */
    if (result == 0)
    {
        result = TINY_ERR_TIMEOUT;
    }
    if ( result < 0 )
    {
        tiny_unlock( handle );
    }
    else
    {
        handle->tx.pframe = 0;
        handle->tx.blockIndex = 0;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////

int tiny_send_buffer(STinyData *handle, uint8_t * pbuf, int len, uint8_t flags)
{
    int result = TINY_NO_ERROR;

    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    if (!handle->tx.pframe && pbuf)
    {
        handle->tx.inprogress = TINY_TX_STATE_SEND_DATA;
        handle->tx.pframe = pbuf;
        handle->tx.sentbytes = 0;
        handle->tx.framelen = len;
    }
    while (1)
    {
        result = __send_frame_data_state(handle);
        if ( result == TINY_SUCCESS )
        {
            handle->tx.pframe = 0;
            handle->tx.blockIndex++;
            handle->tx.sentbytes = 0;
            result = handle->tx.framelen;
            STATS(handle->stat.bytesSent += handle->tx.framelen);
            break;
        }
        /* Repeat cycle if byte is sent successfully */
        if (result == TINY_NO_ERROR)
        {
            continue;
        }
        if (result < 0)
        {
            if ( result == TINY_ERR_BUSY )
            {
                TASK_YIELD();
                result = TINY_NO_ERROR;
            }
            else
            {
                break;
            }
        }
        if (!(flags & TINY_FLAG_WAIT_FOREVER))
        {
            break;
        }
    };
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns TINY_SUCCESS if frame sent is competed
 *         negative value in case of error
 */
int tiny_send_end(STinyData *handle, uint8_t flags)
{
    int result = TINY_NO_ERROR;

    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    while (1)
    {
        if (handle->tx.inprogress == TINY_TX_STATE_SEND_CRC)
        {
            result = __send_frame_crc_state(handle);
        }
        if (handle->tx.inprogress == TINY_TX_STATE_END)
        {
            result = __send_frame_flag_state(handle);
        }
        if (handle->tx.inprogress == TINY_TX_STATE_IDLE)
        {
            STATS(handle->stat.framesSent++);
            tiny_unlock(handle);
            result = TINY_SUCCESS;
            break;
        }
        /* Repeat if byte is sent successfully */
        if (result > 0)
        {
            result = 0;
            continue;
        }
        /* Exit if error */
        if (result < 0)
        {
            if ( result == TINY_ERR_BUSY )
            {
                TASK_YIELD();
                result = TINY_NO_ERROR;
            }
            else
            {
                break;
            }
        }
        if (!(flags & TINY_FLAG_WAIT_FOREVER))
        {
            break;
        }
    };
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////

void tiny_send_terminate(STinyData *handle)
{
    if (!handle)
    {
        return;
    }
    if (handle->tx.inprogress == TINY_TX_STATE_IDLE)
    {
        return;
    }
    tiny_unlock(handle);
    handle->tx.inprogress = TINY_TX_STATE_IDLE;
}

//////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns negative value in case of error
 *         length of sent data
 */
int tiny_send(STinyData *handle, uint16_t *uid, uint8_t * pbuf, int len, uint8_t flags)
{
    int result = TINY_NO_ERROR;
    if ( flags & (TINY_FLAG_LOCK_SEND|TINY_FLAG_WAIT_FOREVER) )
    {
        result = tiny_send_start(handle, flags);
        if (result == TINY_SUCCESS)
        {
            result = TINY_NO_ERROR;
            handle->tx.blockIndex++;
            if (!uid)
            {
                /* Skip sending of uid if not specified */
                handle->tx.blockIndex++;
            }
        }
        else
        {
            return result;
        }
    }
    if ( handle->tx.blockIndex == 1 )
    {
        result = tiny_send_buffer( handle, (uint8_t *)uid, sizeof(uint16_t), flags );
    }
    if ( handle->tx.blockIndex == 2 )
    {
        result = tiny_send_buffer( handle, pbuf, len, flags );
    }
    if ( handle->tx.blockIndex == 3 )
    {
        result = tiny_send_end(handle, flags);
        if (result == TINY_SUCCESS)
        {
            if (len != 0)
            {
                result = len;
            }
            /* If write callback is not null and frame is sent */
            if ( handle->write_cb && (result>0) )
            {
                 handle->write_cb(handle, uid ? *uid : 0, pbuf, len);
            }
        }
    }
    if ( ( result < 0 ) && ( result != TINY_ERR_TIMEOUT ) )
    {
         tiny_send_terminate( handle );
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////

int tiny_simple_send(STinyData *handle, uint8_t *pbuf, int len)
{
    return tiny_send(handle, 0, pbuf, len, TINY_FLAG_WAIT_FOREVER | TINY_FLAG_LOCK_SEND );
}

//////////////////////////////////////////////////////////////////////////////////////


/**************************************************************
*
*                 RECEIVE FUNCTIONS
*
***************************************************************/

int tiny_on_rx_byte(STinyData * handle, uint8_t *pbuf, int len, uint8_t byte)
{
    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    if ( handle->rx.inprogress == TINY_RX_STATE_IDLE )
    {
        /* New frame must be started with 0x7E char */
        if (byte != FLAG_SEQUENCE)
        {
            STATS(handle->stat.oosyncBytes++);
            return TINY_ERR_OUT_OF_SYNC;
        }
        /* Ok, we're at the beginning of new frame. Init state machine. */
        handle->rx.prevbyte = byte;
        handle->rx.framelen = 0;
        handle->rx.bits = 0;
        handle->rx.inprogress = TINY_RX_STATE_READ_DATA;
        __init_fcs_field(handle->fcs_bits, &handle->rx.fcs);
        return TINY_SUCCESS;
    }
    /* If next byte after 0x7E is 0x7E, tiny data have wrong frame alignment.
       Start to read new frame and register error. */
    if ((byte == FLAG_SEQUENCE) && (handle->rx.prevbyte == FLAG_SEQUENCE))
    {
        STATS(handle->stat.framesBroken++);
        /* Read next byte */
        return TINY_ERR_OUT_OF_SYNC;
    }
    /* If received byte is 0x7E, this means end of frame */
    if ((byte == FLAG_SEQUENCE) && (handle->rx.prevbyte != FLAG_SEQUENCE))
    {
        /* End of frame */
        /* Tell that we have done and ready to receive next frame. */
        handle->rx.inprogress = TINY_RX_STATE_IDLE;
        /* Frame is too short. There should be at least 16-bit FCS field. */
        if (handle->rx.framelen < (handle->fcs_bits >> 3))
        {
            STATS(handle->stat.framesBroken++);
            return TINY_ERR_FAILED;
        }
        handle->rx.framelen -= (handle->fcs_bits >> 3); // subtract number of bytes of FCS field
        if (handle->rx.framelen>=len)
        {
            STATS(handle->stat.framesBroken++);
            return TINY_ERR_DATA_TOO_LARGE;
        }
        /* Check CRC */
        if (!__check_fcs_field(handle->fcs_bits, handle->rx.fcs))
        {
            STATS(handle->stat.framesBroken++);
            return TINY_ERR_FAILED;
        }
        /* Call read callback if callback is defined */
        if (handle->read_cb)
        {
            if (handle->uid_support)
            {
                handle->read_cb(handle,
                                *((uint16_t *)pbuf),
                                pbuf + sizeof(uint16_t),
                                handle->rx.framelen - sizeof(uint16_t));
            }
            else
            {
                handle->read_cb(handle,
                                0,
                                pbuf,
                                handle->rx.framelen);
            }
        }
        STATS(handle->stat.bytesReceived += handle->rx.framelen);
        STATS(handle->stat.framesReceived++);
        return TINY_SUCCESS;
    }
    /* If escape char is received - wait for second char */
    if (byte == TINY_ESCAPE_CHAR)
    {
        /* Remember last byte received */
        handle->rx.prevbyte = byte;
        /* Read next byte */
        return TINY_NO_ERROR;
    }
    /* If previous byte is 0x7D, we should decode the byte after */
    if (TINY_ESCAPE_CHAR == handle->rx.prevbyte)
    {
        /* Remember last byte received */
        handle->rx.prevbyte = byte;
        byte ^=  TINY_ESCAPE_BIT; // Update byte and continue
    }
    else
    {
        /* Just remember last byte */
        handle->rx.prevbyte = byte;
    }
    switch (handle->rx.inprogress)
    {
        case TINY_RX_STATE_READ_DATA:
            /* if there is place in buffer save byte.
               2-byte FCS field support is responsibility of Tiny and not visible to upper layer.
               Thus we do not need to have free place for last 2 bytes. */
            if (handle->rx.framelen < len)
            {
                pbuf[handle->rx.framelen] = byte;
            }
            handle->rx.framelen++;
            break;
        default:
            break;
    }
    __update_fcs_field(handle->fcs_bits, &handle->rx.fcs, byte);
    return TINY_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////

int tiny_read_start(STinyData * handle, uint8_t flags)
{
    int                 result = TINY_NO_ERROR;
    uint8_t             byte;

    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    if (!handle->read_func)
    {
        return TINY_ERR_INVALID_DATA;
    }
    if ( handle->rx.inprogress != TINY_RX_STATE_IDLE )
    {
        return TINY_ERR_BUSY;
    }
    do
    {
        result = handle->read_func(handle->pdata, &byte, 1);
        /* Byte is received */
        if (result > 0)
        {
            /* New frame must be started with 0x7E char */
            if (byte != FLAG_SEQUENCE)
            {
                STATS(handle->stat.oosyncBytes++);
                result = TINY_ERR_OUT_OF_SYNC;
                break;
            }
            /* Ok, we're at the beginning of new frame. Init state machine. */
            handle->rx.prevbyte = byte;
            __init_fcs_field(handle->fcs_bits, &handle->rx.fcs);
            result = TINY_SUCCESS;
            break;
        }
        else if (result<0)
        {
            result = TINY_ERR_FAILED;
            break;
        }
        TASK_YIELD();
    } while (flags & TINY_FLAG_WAIT_FOREVER);
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

int tiny_read_buffer(STinyData *handle, uint8_t *pbuf, int len, uint8_t flags)
{
    uint8_t byte = 0;
    int result;
    if (handle->rx.inprogress == TINY_RX_STATE_START)
    {
        handle->rx.framelen = 0;
        handle->rx.bits = 0;
        handle->rx.inprogress = TINY_RX_STATE_READ_DATA;
    }
    while (1)
    {
        result = handle->read_func(handle->pdata, &byte, 1);
        /* Exit if  error */
        if (result<0)
        {
            break;
        }
        /* Exit if no data awaiting */
        if (result == 0)
        {
            if (!(flags & TINY_FLAG_WAIT_FOREVER))
            {
                break;
            }
            TASK_YIELD();
            continue;
        }

        /* If next byte after 0x7E is 0x7E, tiny data have wrong frame alignment.
           Start to read new frame and register error. */
        if ((byte == FLAG_SEQUENCE) && (handle->rx.prevbyte == FLAG_SEQUENCE))
        {
            STATS(handle->stat.framesBroken++);
            /* Read next byte */
            continue;
        }
        /* If received byte is 0x7E, this means end of frame */
        if ((byte == FLAG_SEQUENCE) && (handle->rx.prevbyte != FLAG_SEQUENCE))
        {
            /* End of frame */
            /* Tell that we have done and ready to receive next frame. */
            handle->rx.inprogress = TINY_RX_STATE_IDLE;
            /* Frame is too short. There should be at least 16-bit FCS field. */
            if (handle->rx.framelen < (handle->fcs_bits >> 3))
            {
                STATS(handle->stat.framesBroken++);
                result = TINY_ERR_FAILED;
                break;
            }
            handle->rx.framelen -= (handle->fcs_bits >> 3); // subtract number of bytes of FCS field
            if (handle->rx.framelen>=len)
            {
                STATS(handle->stat.framesBroken++);
                result = TINY_ERR_DATA_TOO_LARGE;
                break;
            }
            /* Check CRC */
            if (!__check_fcs_field(handle->fcs_bits, handle->rx.fcs))
            {
                STATS(handle->stat.framesBroken++);
                result = TINY_ERR_FAILED;
                break;
            }
            STATS(handle->stat.bytesReceived += handle->rx.framelen);
            STATS(handle->stat.framesReceived++);
            result = handle->rx.framelen;
            break;
        }
        /* If escape char is received - wait for second char */
        if (byte == TINY_ESCAPE_CHAR)
        {
            /* Remember last byte received */
            handle->rx.prevbyte = byte;
            /* Read next byte */
            continue;
        }
        /* If previous byte is 0x7D, we should decode the byte after */
        if (TINY_ESCAPE_CHAR == handle->rx.prevbyte)
        {
            /* Remember last byte received */
            handle->rx.prevbyte = byte;
            byte ^=  TINY_ESCAPE_BIT; // Update byte and continue
        }
        else
        {
            /* Just remember last byte */
            handle->rx.prevbyte = byte;
        }
        switch (handle->rx.inprogress)
        {
        case TINY_RX_STATE_READ_DATA:
            /* if there is place in buffer save byte.
               2-byte FCS field support is responsibility of Tiny and not visible to upper layer.
               Thus we do not need to have free place for last 2 bytes. */
            if (handle->rx.framelen < len)
            {
                pbuf[handle->rx.framelen] = byte;
            }
            handle->rx.framelen++;
            break;
        default:
            break;
        }
        __update_fcs_field(handle->fcs_bits, &handle->rx.fcs, byte);
        if ( (handle->rx.framelen == len) && !( flags & TINY_FLAG_READ_ALL ) )
        {
            // Buffer is full
            result = TINY_ERR_DATA_TOO_LARGE;
            handle->rx.inprogress = TINY_RX_STATE_START;
            STATS(handle->stat.bytesReceived += handle->rx.framelen);
            break;
        }
    }
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

void tiny_read_terminate(STinyData *handle)
{
    if (!handle)
    {
        return;
    }
    handle->rx.inprogress = TINY_RX_STATE_IDLE;
}

/////////////////////////////////////////////////////////////////////////////////////////

int tiny_read(STinyData *handle, uint16_t *uid, uint8_t *pbuf, int len, uint8_t flags)
{
    int result = TINY_NO_ERROR;
    if (handle->rx.inprogress == TINY_TX_STATE_IDLE)
    {
        result = tiny_read_start( handle, flags );
        if (result == TINY_SUCCESS )
        {
            handle->rx.inprogress = TINY_RX_STATE_START;
            handle->rx.blockIndex = 1;
            if (!uid) handle->rx.blockIndex++;
            result = TINY_NO_ERROR;
        }
    }
    if (handle->rx.inprogress != TINY_TX_STATE_IDLE)
    {
        if ( handle->rx.blockIndex == 1 )
        {
            result = tiny_read_buffer( handle, (uint8_t *)uid, sizeof(uint16_t), flags );
            if ( result == TINY_ERR_DATA_TOO_LARGE )
            {
                handle->rx.blockIndex++;
            }
        }
        if ( handle->rx.blockIndex == 2 )
        {
            result = tiny_read_buffer( handle, pbuf, len, flags | TINY_FLAG_READ_ALL );
            /* Call read callback if callback is defined */
            if ( (result > 0) && (handle->read_cb) )
            {
                if (uid)
                {
                    handle->read_cb(handle,
                                    *uid,
                                    pbuf,
                                    result);
                }
                else
                {
                    handle->read_cb(handle,
                                    0,
                                    pbuf,
                                    result);
                }
            }

        }
        if ( result < 0 )
        {
            tiny_read_terminate( handle );
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////

int tiny_simple_read(STinyData *handle, uint8_t *pbuf, int len)
{
    return tiny_read(handle, 0, pbuf, len, TINY_FLAG_WAIT_FOREVER);
}


/**************************************************************
*
*                 SERVICE FUNCTIONS
*
***************************************************************/

#ifdef CONFIG_ENABLE_STATS

int tiny_get_stat(STinyData *handle, STinyStats *stat)
{
    if (!handle)
        return TINY_ERR_INVALID_DATA;
    *stat = handle->stat;
    return TINY_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////

int tiny_clear_stat(STinyData *handle)
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

#endif /* CONFIG_ENABLE_STATS */

////////////////////////////////////////////////////////////////////////////////////////////

int tiny_set_callbacks(STinyData *handle,
               on_frame_cb_t read_cb,
               on_frame_cb_t send_cb)
{
    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    tiny_lock( handle, TINY_FLAG_WAIT_FOREVER );
    if ( 0 != read_cb )
    {
        handle->read_cb = read_cb;
    }
    if ( 0 != send_cb )
    {
        handle->write_cb = send_cb;
    }
    tiny_unlock( handle );
    return TINY_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////

int tiny_get_callbacks(STinyData *handle,
               on_frame_cb_t *read_cb,
               on_frame_cb_t *send_cb)
{
    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    tiny_lock( handle, TINY_FLAG_WAIT_FOREVER );
    if ( 0 != read_cb )
    {
        *read_cb = handle->read_cb;
    }
    if ( 0 != send_cb )
    {
        *send_cb = handle->write_cb;
    }
    tiny_unlock( handle );
    return TINY_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////

int tiny_lock(STinyData *handle, uint8_t flags)
{
    if (flags & TINY_FLAG_WAIT_FOREVER)
    {
         MUTEX_LOCK(handle->send_mutex);
         handle->tx.locked = 1;
         return TINY_SUCCESS;
    }
    if (MUTEX_TRY_LOCK(handle->send_mutex) == 0)
    {
         handle->tx.locked = 1;
         return TINY_SUCCESS;
    }
    return TINY_ERR_FAILED;
}

////////////////////////////////////////////////////////////////////////////////////////////

void tiny_unlock(STinyData *handle)
{
    if (handle->tx.locked)
    {
        handle->tx.locked = 0;
        MUTEX_UNLOCK(handle->send_mutex);
    }
}


