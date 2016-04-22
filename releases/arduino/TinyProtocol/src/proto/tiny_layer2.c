/*
    Copyright 2016 (C) Alexey Dynda

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
*   8    16     any len      16/32   8
* | 7E | UID |  USER DATA  |  FCS  | 7E |
*
* 7E is standard frame delimiter (commonly use on layer 2)
* FCS is standard checksum. Refer to RFC1662 for example.
* UID is reserved for future usage
*************************************************************/

#define FLAG_SEQUENCE            0x7E
#define ADDRESS_FIELD            0xFF
#define CONTROL_FILED            0x03
#define PROTOCOL_FIELD           0x00 // 8 or 16 bit
#define TINY_ESCAPE_CHAR         0x7D
#define TINY_ESCAPE_BIT          0x20


/**************************************************************
*
*                 FCS/CHKSUM FUNCTIONS
*
***************************************************************/

#ifdef TINY_FCS_ENABLE
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

inline static int __check_fcs_field(uint8_t fcs_bits, fcs_t fcs)
{
    /* Check CRC */
#ifdef CONFIG_ENABLE_FCS32
    if ((fcs_bits == 32) && (fcs == PPPGOODFCS32)) return 1;
#endif
#ifdef CONFIG_ENABLE_FCS16
    if ((fcs_bits == 16) && (fcs == PPPGOODFCS16)) return 1;
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
    if ((fcs_bits == 8) && (fcs == GOODCHECKSUM)) return 1;
#endif
    return fcs_bits == 0;
}


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


inline static void  __commit_fcs_field(uint8_t fcs_bits, fcs_t* fcs)
{
    /* CRC is calculated only over real user data and frame header */
    switch (fcs_bits)
    {
#ifdef CONFIG_ENABLE_FCS16
    case 16:
        *fcs = *fcs^0xFFFFFFFF; break;
#endif
#ifdef CONFIG_ENABLE_FCS32
    case 32:
        *fcs = *fcs^0xFFFFFFFF; break;
#endif
#ifdef CONFIG_ENABLE_CHECKSUM
    case 8:
        *fcs = 0xFFFF - *fcs; break;
#endif
    default:
        break;
    }
}

#else

#define __init_fcs_field(x, y)

#define __check_fcs_field(x, y)   1

#define __update_fcs_field(x, y, z)

#define __commit_fcs_field(x, y)

#endif

/**************************************************************
*
*                 OPEN/CLOSE FUNCTIONS
*
***************************************************************/


/**
* The function initializes internal structures for Tiny channel and return handle
* to be used with all Tiny and IPC functions.
* @param handle - pointer to Tiny data
* @param write_func - pointer to write data function (to communication channel).
* @param read_func - pointer to read function (from communication channel).
* @param pdata - pointer to a user private data.
* @see write_block_cb_t
* @see read_block_cb_t
* @return TINY_NO_ERROR or error code.
*/
#ifndef ARDUINO_NANO
int tiny_init(STinyData *handle,
              write_block_cb_t write_func,
              read_block_cb_t read_func,
              void *pdata)
#else
int tiny_init(STinyData *handle,
              write_block_cb_t write_func,
              read_block_cb_t read_func)
#endif
{
    if (!handle || !write_func || !read_func)
    {
        return TINY_ERR_INVALID_DATA;
    }
#ifndef ARDUINO_NANO
    handle->pdata = pdata;
#endif
    handle->write_func = write_func;
    handle->read_func = read_func;
#ifdef CONFIG_ENABLE_STATS
    handle->read_cb = 0;
    handle->write_cb = 0;
#endif
#ifdef TINY_FCS_ENABLE
    #if defined(CONFIG_ENABLE_FCS32)
        handle->fcs_bits = 32;
    #elif defined(CONFIG_ENABLE_FCS16)
        handle->fcs_bits = 16;
    #else
        handle->fcs_bits = 8;
    #endif
#endif
    handle->rx.inprogress = TINY_RX_STATE_IDLE;
    handle->tx.inprogress = TINY_TX_STATE_IDLE;
    handle->tx.pframe = 0;
    MUTEX_INIT(handle->send_mutex);
    COND_INIT(handle->send_condition);
#ifdef CONFIG_ENABLE_STATS
    tiny_clear_stat(handle);
#endif
    return TINY_NO_ERROR;
}


int tiny_close(STinyData *handle)
{
    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    MUTEX_LOCK(handle->send_mutex);
    if (handle->rx.inprogress)
    {
        handle->rx.inprogress = TINY_RX_STATE_IDLE;
    }
    if (handle->tx.inprogress)
    {
        handle->tx.inprogress = TINY_TX_STATE_IDLE;
    }
    COND_DESTROY(handle->send_condition);
    MUTEX_UNLOCK(handle->send_mutex);
    MUTEX_DESTROY(handle->send_mutex);
    handle->write_func = 0;
    handle->read_func = 0;
    return TINY_NO_ERROR;
}


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
#ifdef TINY_FCS_ENABLE
    case 0:
        handle->fcs_bits = bits;
        break;
#endif
    default:
        result = TINY_ERR_FAILED;
        break;
    }
    return result;
}


/**************************************************************
*
*                 SEND FUNCTIONS
*
***************************************************************/

#define DATA_BYTE_SENT  2
#define BITS_NO_UID     0
#define BITS_UID_16     16

static int __send_frame_flag_state(STinyData *handle)
{
    uint8_t tempbuf[1];
    int result;
    /* Start frame transmission */
    tempbuf[0] = FLAG_SEQUENCE;
#   ifndef ARDUINO_NANO
    result = handle->write_func(handle->pdata, tempbuf, 1);
#   else
    result = handle->write_func(tempbuf, 1);
#   endif
    if (result < 0)
    {
        return TINY_ERR_FAILED;
    }
    if (result == 0)
        return TINY_NO_ERROR;
    if (handle->tx.inprogress == TINY_TX_STATE_START)
    {
         handle->tx.sentbytes = 0;
         handle->tx.prevbyte = 0;
         __init_fcs_field(handle->fcs_bits, &handle->tx.fcs);
         if (handle->tx.bits == BITS_UID_16)
         {
             handle->tx.inprogress = TINY_TX_STATE_SEND_UID;
             handle->tx.bits = 0;
         }
         else if (handle->tx.bits == BITS_NO_UID)
         {
             handle->tx.inprogress = TINY_TX_STATE_SEND_DATA;
             handle->tx.bits = 0;
         }
    }
    else
    {
         handle->tx.inprogress = TINY_TX_STATE_IDLE;
    }
    return result;
}


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
#   ifndef ARDUINO_NANO
    result = handle->write_func(handle->pdata, &byte, 1);
#   else
    result = handle->write_func(&byte, 1);
#   endif
    if (result <= 0)
        return result;
    /* We calculate crc only for user data */
    handle->tx.prevbyte = byte;
    return byte != TINY_ESCAPE_CHAR;
}


static int __send_frame_uid_state(STinyData *handle)
{
    int result;
    uint8_t byte;
    byte = (handle->tx.uid >> handle->tx.bits) & 0x000000FF;
    result = __send_byte_state(handle, byte);
    if (result > 0)
    {
        __update_fcs_field(handle->fcs_bits, &handle->tx.fcs, byte);
        handle->tx.bits += 8;
        if (handle->tx.bits == 16)
        {
            handle->tx.inprogress = TINY_TX_STATE_SEND_DATA;
        }
    }
    return result;
}


/**
 * This is handler for SEND DATA state. It switches to
 * TINY_TX_STATE_END only if all bytes are sent.
 * If next byte is successfully sent, it returns 1.
 * If switched to other state, it returns 1.
 * If no byte is sent during the cycle, it returns 0.
 * If error happened, it returns negative value.
 * @param handle - pointer to Tiny Protocol structure
 */
static int __send_frame_data_state(STinyData *handle)
{
    int result;
    uint8_t byte;
    if (handle->tx.sentbytes >= handle->tx.framelen)
    {
        __commit_fcs_field(handle->fcs_bits, &handle->tx.fcs);
#ifdef TINY_FCS_ENABLE
        /* sending crc */
        handle->tx.inprogress = handle->fcs_bits ? TINY_TX_STATE_SEND_CRC : TINY_TX_STATE_END;
        handle->tx.bits = 0;
#else
        handle->tx.inprogress = TINY_TX_STATE_END;
#endif
        return 1;
    }
    byte = handle->tx.pframe[handle->tx.sentbytes];
    result = __send_byte_state(handle, byte);
    if (result > 0)
    {
        __update_fcs_field(handle->fcs_bits, &handle->tx.fcs, byte);
        handle->tx.sentbytes++;
    }
    return result;
}

#ifdef TINY_FCS_ENABLE
static int __send_frame_crc_state(STinyData *handle)
{
    int result;
    uint8_t byte;
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
#endif


/**
* __tiny_send_data function sends speficied data to serial port using Tiny
* frame format: 0x7E, data..., FCS, 0x7E.
* @param handle - pointer to Tiny Protocol structure
* @see TINY_ERR_INVALID_DATA
* @see TINY_ERR_FAILED
* @return TINY_ERR_INVALID_DATA, TINY_ERR_FAILED, 0 if HW is not ready or number of sent bytes.
*/
static int __tiny_send_data(STinyData *handle)
{
    int result = TINY_NO_ERROR;

    while (handle->tx.inprogress != TINY_TX_STATE_IDLE)
    {
        switch (handle->tx.inprogress)
        {
        case TINY_TX_STATE_START:
            result = __send_frame_flag_state(handle);
            break;
        case TINY_TX_STATE_END:
            result = __send_frame_flag_state(handle);
            break;
        case TINY_TX_STATE_SEND_UID:
            result = __send_frame_uid_state(handle);
            break;
        case TINY_TX_STATE_SEND_DATA:
            result = __send_frame_data_state(handle);
            break;
#ifdef TINY_FCS_ENABLE
        case TINY_TX_STATE_SEND_CRC:
            result = __send_frame_crc_state(handle);
            break;
#endif
        default:
            break;
        }
        if ( result <= 0 )
        {
            /* Exit in case of error or no data sent */
            return result;
        }
    }

    STATS(handle->stat.framesSent++);
    STATS(handle->stat.bytesSent += handle->tx.sentbytes);
    MUTEX_LOCK(handle->send_mutex);
    handle->tx.pframe = 0;
    COND_SIGNAL(handle->send_condition);
    MUTEX_UNLOCK(handle->send_mutex);
    return handle->tx.sentbytes;
}


#ifdef PLATFORM_MUTEX
/**
 * waits until previous send operaton completes and places new buffer for sending
 */
static int __wait_send_complete(STinyData *handle, uint8_t *pbuf, uint8_t flags)
{
    int result = TINY_NO_ERROR;
    /* We need to use mutex to access real hardware */
    MUTEX_LOCK(handle->send_mutex);
    /* Wait until previous send operation is completed. */
    while (handle->tx.inprogress != TINY_TX_STATE_IDLE)
    {
        if (!(flags & TINY_FLAG_WAIT_FOREVER))
        {
            result = TINY_ERR_BUSY;
            MUTEX_UNLOCK(handle->send_mutex);
            break;
        }
        COND_WAIT(handle->send_condition, handle->send_mutex);
    }
    /* If no send operation is in progress, pass new buffer for sending */
    if (result == TINY_NO_ERROR)
    {
        /* Remember pointer to data of the frame to transfer */
        handle->tx.pframe = pbuf;
        handle->tx.inprogress = TINY_TX_STATE_START;
    }
    MUTEX_UNLOCK(handle->send_mutex);
    return result;
}
#else
#define __wait_send_complete(handle, pbuf, flags)  \
                             0; \
                             handle->tx.pframe = pbuf; \
                             handle->tx.inprogress = TINY_TX_STATE_START;
#endif


int tiny_send(STinyData *handle, uint16_t *uid, uint8_t * pbuf, int len, uint8_t flags)
{
    int result = TINY_NO_ERROR;

    if (!handle)
        return TINY_ERR_INVALID_DATA;

    /* If the buffer being sent is some different buffer, wait until operation is finished */
    if (pbuf != handle->tx.pframe)
    {
        result = __wait_send_complete(handle, pbuf, flags);
        if (result == TINY_NO_ERROR)
        {
            /* We can put new packet for the transfer.
               Pointer is already set by _wait_send_complete. */
            if (uid)
            {
                handle->tx.uid = *uid;
                handle->tx.bits = BITS_UID_16;
            }
            else
            {
                handle->tx.bits = BITS_NO_UID;
            }
            handle->tx.pframe = pbuf;
            handle->tx.framelen = len;
        }
    }
    if (result == TINY_NO_ERROR)
    {
        do
        {
            result = __tiny_send_data(handle);
            if ((result != 0) || (handle->tx.inprogress == TINY_TX_STATE_IDLE))
            {
                /* exit on error and on successful send */
                break;
            }
        } while (flags & TINY_FLAG_WAIT_FOREVER);
    }
#ifdef CONFIG_ENABLE_STATS
    /* If write callback is not null and frame is sent */
    if (handle->write_cb && (result>0))
    {
        handle->write_cb(handle, TINY_FRAME_TX, pbuf, len);
    }
#endif
    return result;
}


/**************************************************************
*
*                 RECEIVE FUNCTIONS
*
***************************************************************/


/**
 * __tiny_read_data function receives and decodes bytes from Tiny protocol channel
 * until 0x7E byte is received.
 * If there are no input bytes, function saves its state to handle and returns 0.
 * @param handle a pointer to STinyData structure.
 * @param byte an unsigned char - byte to put into send buffer
 * @param pbuf a pointer to receive buffer
 * @param len an integer argument - maximum receive buffer size
 * @return TINY_ERR_FAILED, TINY_ERR_DATA_TOO_LARGE or number of received bytes.
 *         0 if no packet is not yet completed.
 */
static int __tiny_read_data(STinyData *handle, uint16_t *uid, uint8_t *pbuf, int len)
{
    uint8_t byte = 0;
    int result;
    if (handle->rx.inprogress == TINY_RX_STATE_START)
    {
        handle->rx.uid = 0;
        handle->rx.bits = 0;
        if (uid)
        {
            handle->rx.inprogress = TINY_RX_STATE_READ_UID;
        }
        else
        {
            handle->rx.inprogress = TINY_RX_STATE_READ_DATA;
        }
    }

    while (1)
    {
#       ifndef ARDUINO_NANO
        result = handle->read_func(handle->pdata, &byte, 1);
#       else
        result = handle->read_func(&byte, 1);
#       endif
        /* Exit if UART error */
        if (result<0) return result;
        /* Exit if no data awaiting */
        if (result == 0) return result;

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
        case TINY_RX_STATE_READ_UID:
            handle->rx.uid |= (byte << handle->rx.bits);
            handle->rx.bits += 8;
            if (handle->rx.bits == 16)
            {
                handle->rx.inprogress = TINY_RX_STATE_READ_DATA;
            }
            break;
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
    }
    if (uid)
    {
        *uid = handle->rx.uid;
    }
    /* Tell that we have done and ready to receive next frame. */
    handle->rx.inprogress = TINY_RX_STATE_IDLE;
#ifdef TINY_FCS_ENABLE
    /* Frame is too short. There should be at least 16-bit FCS field. */
    if (handle->rx.framelen < (handle->fcs_bits >> 3))
    {
        STATS(handle->stat.framesBroken++);
        return TINY_ERR_FAILED;
    }
    handle->rx.framelen -= (handle->fcs_bits >> 3); // subtract number of bytes of FCS field
#endif
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
#ifdef CONFIG_ENABLE_STATS
    /* Call read callback if callback is defined */
    if (handle->read_cb)
    {
        handle->read_cb(handle, TINY_FRAME_RX, pbuf, handle->rx.framelen);
    }
#endif
    STATS(handle->stat.bytesReceived += handle->rx.framelen);
    STATS(handle->stat.framesReceived++);
    return handle->rx.framelen;
}


int tiny_read(STinyData *handle, uint16_t *uid, uint8_t *pbuf, int len, uint8_t flags)
{
    int                 result = TINY_NO_ERROR;
    uint8_t               byte;

    if (!handle)
        return TINY_ERR_INVALID_DATA;
    if (!handle->read_func)
        return TINY_ERR_INVALID_DATA;

    do
    {
        /* If no any frame processing is in progress, check for new frame */
        if (handle->rx.inprogress == TINY_RX_STATE_IDLE)
        {
#           ifndef ARDUINO_NANO
            result = handle->read_func(handle->pdata, &byte, 1);
#           else
            result = handle->read_func(&byte, 1);
#           endif
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
                handle->rx.framelen = 0;
                handle->rx.inprogress = TINY_RX_STATE_START;
                handle->rx.prevbyte = byte;
                __init_fcs_field(handle->fcs_bits, &handle->rx.fcs);
            }
            else if (result<0)
            {
                result = TINY_ERR_FAILED;
                break;
            }
        }
        /* Some frame is in process of receiving. Continue to parse data */
        if (handle->rx.inprogress != TINY_RX_STATE_IDLE)
        {
            result = __tiny_read_data(handle, uid, pbuf, len);
        }
        if (result == 0)
        {
            if (!(flags & TINY_FLAG_WAIT_FOREVER))
            {
                break;
            }
            TASK_YIELD();
        }
    }
    while (result == 0);

    return result;
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


/**
* tiny_clear_stat function clears Tiny protocol statistics.
* @param handle - pointer to Tiny protocol structure
* @see TINY_ERR_INVALID_DATA
* @see TINY_NO_ERROR
* @return TINY_ERR_INVALID_DATA, TINY_NO_ERROR.
*/
int tiny_clear_stat(STinyData *handle)
{
    if (handle)
        return TINY_ERR_INVALID_DATA;
    handle->stat.bytesSent = 0;
    handle->stat.bytesReceived = 0;
    handle->stat.framesBroken = 0;
    handle->stat.framesReceived = 0;
    handle->stat.framesSent = 0;
    handle->stat.oosyncBytes = 0;
    return TINY_NO_ERROR;
}


/**
* tiny_set_callbacks sets callback procs for specified Tiny protocol.
* callbacks will receive all data being sent or received.
* @param handle - pointer to Tiny Protocol structure
* @param read_cb - an argument of on_frame_cb_t type - pointer to callback function.
* @param send_cb - an argument of on_frame_cb_t type - pointer to callback function.
* @return TINY_ERR_INVALID_DATA, TINY_NO_ERROR.
*/
int tiny_set_callbacks(STinyData *handle,
               on_frame_cb_t read_cb,
               on_frame_cb_t send_cb)
{
    if (!handle)
    {
        return TINY_ERR_INVALID_DATA;
    }
    MUTEX_LOCK(handle->send_mutex);
    handle->read_cb = read_cb;
    handle->write_cb = send_cb;
    MUTEX_UNLOCK(handle->send_mutex);
    return TINY_NO_ERROR;
}

#endif /* CONFIG_ENABLE_STATS */

