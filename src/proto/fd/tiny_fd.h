/*
    Copyright 2019 (C) Alexey Dynda

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
 @brief Tiny Protocol Full Duplex API
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "proto/hdlc/tiny_hdlc.h"
#include "proto/hal/tiny_types.h"

struct tiny_fd_data_t;

/**
 * This handle points to service data, required for full-duplex
 * functioning.
 */
typedef struct tiny_fd_data_t *tiny_fd_handle_t;

/**
 * This structure is used for initialization of Tiny Full Duplex protocol.
 */
typedef struct STinyFdInit_
{
    /// callback function to write bytes to the physical channel
    write_block_cb_t   write_func;
    /// callback function to read bytes from the physical channel
    read_block_cb_t    read_func;
    /// user data for block read/write functions
    void             * pdata;
    /// callback function to process incoming frames
    on_frame_cb_t      on_frame_cb;
    /// Callback to get notification of sent frames
    on_frame_cb_t      on_sent_cb;
    /**
     * buffer to store data during full-duplex protocol operating.
     * The size should be TBD
     */
    void             * buffer;
    /// maximum input buffer size
    uint16_t           buffer_size;
    /// timeout. Can be set to 0 during initialization. In this case timeout will be set to default
    uint16_t           timeout;
    /// multithread mode. 0 for single thread applications, 1 - for multithread applications
    uint8_t            multithread_mode;
    /**
     * crc field type to use on hdlc level.
     * If HDLC_CRC_DEFAULT is passed, crc type will be selected automatically (depending on library configuration),
     * but HDLC_CRC_16 has higher priority.
     */
    hdlc_crc_t         crc_type;
    /// number of frames in window. Must be at least 2
    uint8_t            window_frames;
} STinyFdInit;

/**
 * @defgroup FULL_DUPLEX_API Tiny Full Duplex API functions
 * @{
 */

/**
 * @brief Initialized communication for Tiny Full Duplex protocol.
 *
 * The function initializes internal structures for Tiny Full Duplex state machine.
 *
 * @param handle - pointer to Tiny Full Duplex data
 * @param init - pointer to STinyFdInit data.
 * @return TINY_NO_ERROR in case of success.
 *         TINY_ERR_FAILED if init parameters are incorrect.
 * @remarks This function is not thread safe.
 */
extern int tiny_fd_init(tiny_fd_handle_t *handle,
                        STinyFdInit      *init);

/**
 * @brief stops Tiny Full Duplex state machine
 *
 * stops Tiny Full Duplex state machine.
 *
 * @param handle - pointer to tiny_fd_handle_t
 */
extern void tiny_fd_close(tiny_fd_handle_t handle);

extern int tiny_fd_run_tx(tiny_fd_handle_t handle, uint16_t timeout);

extern int tiny_fd_run_rx(tiny_fd_handle_t handle, uint16_t timeout);

/**
 * @brief runs receive functions of Tiny Full Duplex protocol.
 *
 * Runs receive functions of Tiny Full Duplex protocol. This function
 * must be called all the time in the loop if send operation is not performed.
 * For atmega controllers this means to call tiny_fd_run() in loop() routine.
 * If user packet is received during execution of the function, it wil
 * call on_frame_cb_t callback to process received packet.
 *
 * @param handle - pointer to tiny_fd_handle_t
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
extern int tiny_fd_run(tiny_fd_handle_t handle);

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
 * @param handle - pointer to tiny_fd_handle_t
 * @param buf    - data to send
 * @param len    - length of data to send
 *
 * @return number of bytes, sent.
 *         TINY_ERR_TIMEOUT  - if no response from remote side after 3 retries.
 *         TINY_ERR_FAILED   - if request was cancelled, by tiny_hd_close().
 *         other error codes - if send/receive errors happened.
 */
extern int tiny_fd_send(tiny_fd_handle_t handle, const void *buf, int len, uint16_t timeout);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
