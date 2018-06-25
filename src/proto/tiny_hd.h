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

/**
 This is Tiny Half-Duplex protocol implementation for microcontrollers.
 It is built on top of Tiny Protocol (tiny_layer2.c)

 @file
 @brief Tiny Protocol Half Duplex API
*/
#ifndef _TINY_HALF_DUPLEX_H_
#define _TINY_HALF_DUPLEX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tiny_layer2.h"


/**
 * This structure contains service data, required for half-duplex
 * functioning.
 */
typedef struct STinyHdData_
{
    /// Original STinyData structure. It is used to control lower level
    STinyData          handle;
    /// Callback to process received frames
    on_frame_cb_t      on_frame_cb;
    /// Buffer to store frames being received
    void          *    inbuf;
    /// maximum size of Rx buffer
    uint16_t           inbuf_size;
    /// Timeout for operations with acknowledge
    uint16_t           timeout;
    /// field used to store temporary uid
    uint16_t           uid;
    /// only single thread mode is supporte now. Should be zero
    uint8_t            multithread_mode;
} STinyHdData;


/**
 * This structure is used for initialization of Tiny Half Duplex protocol.
 */
typedef struct STinyHdInit_
{
    /// callback function to write bytes to the physical channel
    write_block_cb_t   write_func;
    /// callback function to read buytes from the physical channel
    read_block_cb_t    read_func;
    /// user data for block read/write functions
    void             * pdata;
    /// callback function to process incoming frames
    on_frame_cb_t      on_frame_cb;
    /// buffer to store input bytes being received. Must be at least maximum packet size over communication channel
    void             * inbuf;
    /// maximum input buffer size
    uint16_t           inbuf_size;
    /// timeout. Can be set to 0 during initialization. In this case timeout will be set to default
    uint16_t           timeout;
    /// multithread mode. At present should be 0
    uint8_t            multithread_mode;
} STinyHdInit;

/**
 * @defgroup HALF_DUPLEX_API Tiny Half Duplex API functions
 * @{
 */

/**
 * @brief Initialized communication for Tiny Half Duplex protocol.
 *
 * The function initializes internal structures for Tiny Half Duplex state machine.
 *
 * @param handle - pointer to Tiny Half Duplex data
 * @param init - pointer to STinyHdInit data.
 * @return TINY_NO_ERROR in case of success.
 *         TINY_ERR_FAILED if init parameters are incorrect.
 * @remarks This function is not thread safe.
 */
extern int tiny_hd_init(STinyHdData      * handle,
                        STinyHdInit      * init);

/**
 * @brief stops Tiny Half Duplex state machine
 *
 * stops Tiny Half Duplex state machine.
 *
 * @param handle - pointer to STinyHdData
 */
extern void tiny_hd_close(STinyHdData    * handle);

/**
 * @brief runs receive functions of Tiny Half Duplex protocol.
 *
 * Runs receive functions of Tiny Half Duplex protocol. This function
 * must be called all the time in the loop if send operation is not performed.
 * For atmega controllers this means to call tiny_hd_run() in loop() routine.
 * If user packet is received during execution of the function, it wil
 * call on_frame_cb_t callback to process received packet.
 *
 * @param handle - pointer to STinyHdData
 * @see TINY_ERR_FAILED
 * @see TINY_ERR_DATA_TOO_LARGE
 * @see TINY_ERR_OUT_OF_SYNC
 * @see TINY_ERR_BUSY
 * @see TINY_NO_ERROR
 * @return TINY_ERR_OUT_OF_SYNC    - if first byte received from the channel is not packet marker. You may ignore this error.
 *         TINY_ERR_DATA_TOO_LARGE - if incoming packet is too large to fit the buffer
 *         TINY_ERR_FAILED         - if something wrong with received data
 *         TINY_BUSY               - if another receive operation is in progress.
 *         TINY_NO_ERROR           - if nothing is received.
 *         greater 0               - number of received bytes. Callbak will be called in this case.
 * @warning do not use blocking send (tiny_send_wait_ack) in on_frame_cb_t callback.
 */
extern int tiny_hd_run(STinyHdData   * handle);

/**
 * @brief Sends userdata and waits for acknowledgement from remote side.
 *
 * Sends userdata and waits for acknowledgement from remote side.
 *
 * @remarks Each outoing packet is assigned 16-bit uid, and remote side must confirm
 * with service packet sent back with this 16-bit uid.
 * Generated uids have range in [0 : 0x0FFF]. Higher 4 bits of uid field are used
 * to pass flags with the packet.
 *
 * @param handle - pointer to STinyHdData
 * @param buf    - data to send
 * @param len    - length of data to send
 *
 * @return number of bytes, sent.
 *         TINY_ERR_TIMEOUT  - if no response from remote side after 3 retries.
 *         TINY_ERR_FAILED   - if request was cancelled, by tiny_hd_close().
 *         other error codes - if send/receive errors happened.
 */
extern int tiny_send_wait_ack(STinyHdData *handle, void *buf, uint16_t len);


static inline STinyData * hd_to_tiny(STinyHdData      * handle)
{
    return &handle->handle;
}

static inline STinyHdData * tiny_to_hd(STinyData * handle)
{
    return (STinyHdData *)handle;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _TINY_HALFDUPLEX_PROTOCOL_H_ */
