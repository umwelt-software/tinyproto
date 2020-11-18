/*
    Copyright 2019-2020 (C) Alexey Dynda

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
#pragma once

#include "hal/tiny_types.h"
#include "proto/crc/crc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Byte to fill gap between frames */
#define TINY_HDLC_FILL_BYTE            0xFF

/**
 * @defgroup HDLC_API Tiny HDLC protocol API functions
 * @{
 *
 * @brief low level HDLC protocol function - only framing
 *
 * @details this group implements low level HDLC functions, which implement
 *          framing only according to RFC 1662: 0x7E, 0x7D, 0x20 (ISO Standard 3309-1979).
 */

struct tiny_hdlc_data_t;

typedef struct tiny_hdlc_data_t *tiny_hdlc_handle_t;

/**
 * Structure describes configuration of lowest HDLC level
 * Initialize this structure by 0 before passing to tiny_hdlc_init()
 * function.
 */
typedef struct
{
    /**
     * User-defined callback, which is called when new packet arrives from hw
     * channel. The context of this callback is context, where tiny_hdlc_run_rx() is
     * called from.
     * @param user_data user-defined data
     * @param data pointer to received data
     * @param len size of received data in bytes
     * @return user callback must return negative value in case of error
     *         or 0 value if packet is successfully processed.
     */

    int (*on_frame_read)(void *user_data, void *data, int len);

    /**
     * User-defined callback, which is called when the packet is sent to TX
     * channel. The context of this callback is context, where tiny_hdlc_run_tx() is
     * called from.
     * @param user_data user-defined data
     * @param data pointer to sent data
     * @param len size of sent data in bytes
     * @return user callback must return negative value in case of error
     *         or 0 value if packet is successfully processed.
     */
    int (*on_frame_sent)(void *user_data, const void *data, int len);

    /**
     * Buffer to be used by hdlc level to receive data to.
     * Use tiny_hdlc_get_buf_size() to calculate minimum buffer size.
     */
    void *buf;

    /**
     * size of hdlc buffer
     */
    int buf_size;

    /**
     * crc field type to use on hdlc level.
     * If HDLC_CRC_DEFAULT is passed, crc type will be selected automatically (depending on library configuration),
     * but HDLC_CRC_16 has higher priority.
     */
    hdlc_crc_t crc_type;

    /** User data, which will be passed to user-defined callback as first argument */
    void *user_data;
} tiny_hdlc_init_t;

//------------------------ GENERIC FUNCIONS ------------------------------

/**
 * Initializes hdlc level and returns hdlc handle or NULL in case of error.
 *
 * @param init pointer to tiny_hdlc_struct_t structure, which defines user-specific configuration
 * @return -1 if error
 *          0 if success
 */
int tiny_hdlc_init( tiny_hdlc_handle_t *handle, tiny_hdlc_init_t *init );

/**
 * Shutdowns all hdlc activity
 *
 * @param handle handle to hdlc instance
 */
int tiny_hdlc_close( tiny_hdlc_handle_t handle );

/**
 * Resets hdlc state. Use this function, if hw error happened on tx or rx
 * line, and this requires hardware change, and cancelling current operation.
 *
 * @param handle handle to hdlc instance
 */
void tiny_hdlc_reset( tiny_hdlc_handle_t handle );

//------------------------ RX FUNCIONS ------------------------------

/**
 * Processes incoming data. Implementation of reading data from hw is user
 * responsibility. If tiny_hdlc_run_rx() returns value less than size of data
 * passed to the function, then tiny_hdlc_run_rx() must be called later second
 * time with the pointer to and size of not processed bytes.
 *
 * If you don't care about errors on RX line, it is allowed to ignore
 * all error codes except TINY_ERR_FAILED, which means general failure.
 *
 * if tiny_hdlc_run_rx() returns 0 bytes processed, just call it once again.
 * It is guaranteed, that at least second call will process bytes.
 *
 * This function will return the following codes in error field:
 *   - TINY_ERR_DATA_TOO_LARGE if receiving data fails to fit incoming buffer
 *   - TINY_ERR_FAILED if generic failure happened
 *   - TINY_ERR_WRONG_CRC if crc field of incoming frame is incorrect
 *   - TINY_SUCCESS if operation completed successfully
 *
 * @param handle handle to hdlc instance
 * @param data pointer to incoming data to process
 * @param len size of received data in bytes
 * @param error pointer to store error code. If no error, 0 is returned.
 *        this argument can be NULL.
 * @return number of processed bytes from specified data buffer.
 *         Never returns negative value
 */
int tiny_hdlc_run_rx( tiny_hdlc_handle_t handle, const void *data, int len, int *error );

//------------------------ TX FUNCIONS ------------------------------

/**
 * If hdlc protocol has some data to send it will full data with
 * This function returns either if no more data to send, or specified
 * buffer is filled completely.
 *
 * @param handle handle to hdlc instance
 * @param data pointer to buffer to fill with data
 * @param len length of specified buffer
 * @return number of bytes written to specified buffer
 */
int tiny_hdlc_run_tx( tiny_hdlc_handle_t handle, void *data, int len );

/**
 * Puts next frame for sending.
 *
 * tiny_hdlc_put() function will not wait or perform send operation, but only pass data pointer to
 * hdlc state machine. In this case, some other thread needs to
 * or in the same thread you need to send data using tiny_hdlc_get_tx_data().
 *
 * @param handle handle to hdlc instance
 * @param data pointer to new data to send (can be NULL is you need to retry sending)
 * @param len size of data to send in bytes
 * @return TINY_ERR_BUSY if TX queue is busy with another frame.
 *         TINY_ERR_INVALID_DATA if len is zero.
 *         TINY_SUCCESS if data is successfully sent
 * @warning buffer with data must be available all the time until
 *          data are actually sent to tx hw channel. That is if you use
 *          zero timeout, you need to use callback to understand, when buffer
 *          is not longer needed at hdlc level.
 * @note TINY_ERR_BUSY and TINY_ERR_INVALID_DATA refer to putting new frame to TX
 *       hdlc queue.
 */
int tiny_hdlc_put( tiny_hdlc_handle_t handle, const void *data, int len );

int tiny_hdlc_get_buf_size( int max_frame_size );

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

