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

/**
 This is Tiny protocol implementation for microcontrollers

 @file
 @brief Tiny protocol API
*/
#ifndef _TINY_LAYER2_H_
#define _TINY_LAYER2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tiny_defines.h"

/// \cond
#ifdef CONFIG_ENABLE_FCS32
    typedef uint32_t fcs_t;
#else
    typedef uint16_t fcs_t;
#endif
/// \endcond

/**
 * @defgroup ERROR_FLAGS Return error codes for Tiny API functions
 * @{
 */
/// Tiny operation successful. Only tiny_send_start and tiny_read_start functions return this code
#define TINY_SUCCESS                    (1)
/// No error. For tiny_send and tiny_read functions, this means, no data sent or received
#define TINY_NO_ERROR                   (0)
/// Timeout
#define TINY_ERR_FAILED                 (-1)
/// Timeout happened. The function must be called once again.
#define TINY_ERR_TIMEOUT                (-2)
/// Data too large to fit the user buffer, valid for tiny_read function
#define TINY_ERR_DATA_TOO_LARGE         (-3)
/// Some invalid data passed to Tiny API function.
#define TINY_ERR_INVALID_DATA           (-4)
/// API function detected that operation cannot be performed right now.
#define TINY_ERR_BUSY                   (-5)
/// Out of sync - received some data, which are not part of the frame (tiny_read)
#define TINY_ERR_OUT_OF_SYNC            (-6)

/** @} */

/**
 * @defgroup FLAGS_GROUP Flags for Tiny API functions
 * @{
 */

/// This flag makes tiny API functions perform as non-blocking
#define TINY_FLAG_NO_WAIT               (0)
/// This flag makes tiny_read function to read whole frame event if it doesn't fit the buffer
#define TINY_FLAG_READ_ALL              (1)
/// This flag makes tiny API functions perform in blocking mode
#define TINY_FLAG_WAIT_FOREVER          (0x80)

/** @} */

/// \cond
typedef enum
{
    TINY_TX_STATE_IDLE,
    TINY_TX_STATE_START,
    TINY_TX_STATE_SEND_DATA,
    TINY_TX_STATE_SEND_CRC,
    TINY_TX_STATE_END
} ETinyTxState;

typedef enum
{
    TINY_RX_STATE_IDLE,
    TINY_RX_STATE_START,
    TINY_RX_STATE_READ_DATA,
    TINY_RX_STATE_END
} ETinyRxState;
/// \endcond


/**
 * Enum is used for callback functions to specify
 * direction of the frame, processed by the protocol.
 */
typedef enum
{
    TINY_FRAME_TX,
    TINY_FRAME_RX,
} ETinyFrameDirection;

/**
 * The function writes data to communication channel port.
 * @param pdata - pointer to user private data - absent in Arduino version
 * @param buffer - pointer to the data to send to channel.
 * @param size - size of data to write.
 * @see read_block_cb_t
 * @return the function must return negative value in case of error or number of bytes written
 *         or zero.
 */
typedef int (*write_block_cb_t)(void *pdata, const uint8_t *buffer, int size);

/**
 * The function reads data from communication channel.
 * @param pdata - pointer to user private data. - absent in Arduino version
 * @param buffer - pointer to a buffer to read data to from the channel.
 * @param size - maximum size of the buffer.
 * @see write_block_cb_t
 * @return the function must return negative value in case of error or number of bytes actually read
 *         or zero.
 */
typedef int (*read_block_cb_t)(void *pdata, uint8_t *buffer, int size);


/**
 * on_frame_cb_t is a function to callback data going through Tiny level.
 * @param handle - handle of Tiny.
 * @param direction - frame is sent or received.
 * @param pdata - data sent or received over Tiny.
 * @param size - size of data.
 * @return None.
 */
typedef void (*on_frame_cb_t)(void *handle, ETinyFrameDirection direction, uint8_t *pdata, int size);


/**
 * This structure contains captured statistics while the protocol
 * sends and receives messages.
 */
typedef struct
{
    /// Number of bytes received out of frame bytes
    uint32_t            oosyncBytes;
    /// Number of payload bytes totally sent through the channel
    uint32_t            bytesSent;
    /// Number of payload bytes totally received through the channel
    uint32_t            bytesReceived;
    /// Number of frames, successfully sent through the channel
    uint32_t            framesSent;
    /// Number of frames, successfully received through the communication channel
    uint32_t            framesReceived;
    /// Number of broken frames received
    uint32_t            framesBroken;
} STinyStats;


/**
 * This structure describes the state of Receive State machine.
 * \warning This is for internal use only, and should not be accessed directly from the application.
 */
typedef struct
{
    /// Number of bytes in the frame being received.
    /** This value becomes valid only when TINY_RX_STATE_END is reached. */
    int                 framelen;
    /// The state of Receive State machine
    uint8_t             inprogress;
    /// The field contains calculated checksum and not available in TINY_MINIMAL configuration
    fcs_t               fcs;
    /// \cond
    uint8_t             blockIndex;
    uint8_t             bits;
    uint8_t             prevbyte;
    /// \endcond
} STinyRxStatus;


/**
 * This structure describes the state of Transmit State machine.
 * \warning This is for internal use only, and should not be accessed directly from the application.
 */
typedef struct
{
    /// Number of payload bytes, sent from the frame being sent
    int                 sentbytes;
    /// Pointer to the frame data being sent
    uint8_t             *pframe;
    /// Length of the frame passed by a user
    int                 framelen;
    /// @see ETinyTxState
    uint8_t             inprogress;
    /// The field contains calculated checksum and not available in TINY_MINIMAL configuration
    fcs_t               fcs;                       // fcs field
    /// \cond
    uint8_t             blockIndex;                // index of the frame block being processed
    uint8_t             prevbyte;                  // last byte sent
    uint8_t             bits;                      // index of crc byte being sent
    /// \endcond
} STinyTxStatus;

/*************************************************************
*
*          Tiny defines
*
**************************************************************/

/**
 * This structure contains information about communication channel and its state.
 * \warning This is for internal use only, and should not be accessed directly from the application.
 */
typedef struct
{
    /// pointer to platform related write function
    write_block_cb_t    write_func;
    /// pointer to platform related read function
    read_block_cb_t     read_func;
    /// pointer to application defined data, passed during protocol initialization - absent in Arduino version
    void*               pdata;
    /// @see STinyRxStatus
    STinyRxStatus       rx;
    /// @see STinyTxStatus
    STinyTxStatus       tx;
#ifdef PLATFORM_MUTEX
    PLATFORM_MUTEX      send_mutex;                // Mutex for send operation
#endif
#ifdef PLATFORM_COND
    PLATFORM_COND       send_condition;            // Condition is called, when send operation is completed
#endif
    /// The field contains number of bits to use for FCS and not available in TINY_MINIMAL configuration
    uint8_t             fcs_bits;
#ifdef CONFIG_ENABLE_STATS
    /// @see STinyStats
    STinyStats          stat;
    /// pointer to callback function
    /** This callback is called when it is not null and when
        new frame is successfully received.
     */
    on_frame_cb_t       read_cb;
    /// pointer to callback function to get data being sent
    /** This callback is called when it is not null and when
        new frame is successfully sent. */
    on_frame_cb_t       write_cb;
#endif
} STinyData;


/**
 * @defgroup SIMPLE_API Tiny simple API functions
 * @{
 */

/**
* The function initializes internal structures for Tiny channel and return handle
* to be used with all Tiny and IPC functions.
* @param handle - pointer to Tiny data
* @param write_func - pointer to write data function (to communication channel).
* @param read_func - pointer to read function (from communication channel).
* @param pdata - pointer to a user private data.
* @note pdata parameter is not applicable for Arduino and is absent in its version.
* @see write_block_cb_t
* @see read_block_cb_t
* @return TINY_NO_ERROR or error code.
*/
extern int tiny_init(STinyData *handle,
               write_block_cb_t write_func,
               read_block_cb_t read_func,
               void *pdata);

/**
* The function closes  channel.
* @param handle - pointer to Tiny data.
* @see tiny_init()
* @return TINY_ERR_INVALID_DATA, TINY_NO_ERROR.
*/
extern int tiny_close(STinyData *handle);


/**
 * @brief sends frame with user payload to communication channel
 *
 * The function sends data to communication channel in the following
 * frame format: 0x7E, data..., FCS, 0x7E.
 * \note if flags field is set to TINY_FLAG_NO_WAIT, then this function may remember pbuf
 *       pointer and return immediately. So, it is responsibility of the caller to make
 *       pbuf data to be available all the time until frame is sent.
 * @param handle - pointer to Tiny data.
 * @param uid - pointer to 16-bit variable containing packet uid, can be NULL. uid value must be spelled in network order bytes.
 * @param pbuf - a const pointer to unsigned char - buffer with data to send
 * @param len - an integer argument - length of data to send
 * @param flags - TINY_FLAG_NO_WAIT
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_FAILED
 * @see TINY_FLAG_NO_WAIT
 * @see TINY_FLAG_WAIT_FOREVER
 * @return TINY_ERR_INVALID_DATA, TINY_ERR_FAILED or number of sent bytes.
 */
extern int tiny_send(STinyData *handle, uint16_t *uid, uint8_t * pbuf, int len, uint8_t flags);


/**
 * The function reads data from communication channel
 * frame format: 0x7E, data..., FCS, 0x7E.
 * @param handle - pointer to Tiny data.
 * @param uid - pointer to 16-bit uid variable to fill with packet info, can be NULL.
 * @param pbuf a const pointer to unsigned char - buffer with data to send
 * @param len an integer argument - length of data to send
 * @param flags - TINY_FLAG_NO_WAIT
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_FAILED
 * @see TINY_ERR_DATA_TOO_LARGE
 * @see TINY_ERR_OUT_OF_SYNC
 * @see TINY_ERR_BUSY
 * @return TINY_ERR_INVALID_DATA, TINY_ERR_FAILED or number of sent bytes.
 */
extern int tiny_read(STinyData *handle, uint16_t *uid, uint8_t *pbuf, int len, uint8_t flags);


/**
 * @brief sends frame with user payload to communication channel in blocking mode
 *
 * The function sends data to communication channel.
 * Unlike tiny_send(), the function works in blocking mode, i.e. it returns
 * control only if user data are successfully sent, or in case of error.
 * @param handle - pointer to Tiny data.
 * @param pbuf - a const pointer to unsigned char - buffer with data to send
 * @param len - an integer argument - length of data to send
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_FAILED
 * @return TINY_ERR_INVALID_DATA, TINY_ERR_FAILED or number of sent bytes.
 */
extern int tiny_simple_send(STinyData *handle, uint8_t *pbuf, int len);


/**
 * @brief reads frame from the channel in blocking mode.
 *
 * The function reads user data from communication channel
 * @param handle - pointer to Tiny data.
 * @param pbuf a const pointer to unsigned char - buffer with data to send
 * @param len an integer argument - length of data to send
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_FAILED
 * @see TINY_ERR_DATA_TOO_LARGE
 * @see TINY_ERR_OUT_OF_SYNC
 * @see TINY_ERR_BUSY
 * @return TINY_ERR_INVALID_DATA, TINY_ERR_FAILED, TINY_ERR_OUT_OF_SYNC, TINY_ERR_BUSY, TINY_ERR_DATA_TOO_LARGE
 *         or number of sent bytes.
 * @note TINY_ERR_DATA_TOO_LARGE can be returned in successful case. If frame is received, but passed buffer
 *       to the function is too small to fit all.
 */
extern int tiny_simple_read(STinyData *handle, uint8_t *pbuf, int len);

/**
 * @}
 */

/**
 * @defgroup ADVANCED_API Tiny advanced API functions
 * @{
 */

/**
 * The function sets number of bits used for fcs
 * @param handle - pointer to Tiny structure
 * @param bits - number of bits to use for fcs: 16 or 32.
 * @return TINY_ERR_FAILED
 *         TINY_NO_ERROR
 */
extern int tiny_set_fcs_bits(STinyData *handle, uint8_t bits);


/**
 * @brief initiates sending of a new frame
 *
 * The function initiates sending of a new frame by writing frame start marker to
 * communication channel. If the function is executed successfully, then
 * tiny_send_buffer and tiny_send_end can be used to pass the data.
 *
 * @param handle - pointer to Tiny data.
 * @param flags - TINY_FLAG_NO_WAIT or TINY_FLAG_WAIT_FOREVER
 * @return TINY_SUCCESS if new frame transmission is started successfully
 *         TINY_ERR_TIMEOUT if non-blocking operation is requested and the channel is busy
 *         TINY_ERR_FAILED if writing to the channel failed
 *         TINY_ERR_INVALID_DATA if invalid handle is passed
 *
 * @see TINY_SUCCESS
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_TIMEOUT
 * @see TINY_ERR_FAILED
 * @see TINY_FLAG_NO_WAIT
 * @see TINY_FLAG_WAIT_FOREVER
 */
extern int tiny_send_start(STinyData *handle, uint8_t flags);

/**
 * @brief sends user provided data in the body of the frame
 *
 * The function sends user provided data (payload) in the body of the frame.
 * It is possible to send several buffers if the single frame. In this case
 * the receiver side will receive all buffers as one data block. Extended read
 * functions allow to read data of the frame to several buffers.
 * If function is in non-blocking mode, it may return immediately with
 * TINY_NO_ERROR. In this case a user should call this function later with the
 * same parameters.

 * \note if flags field is set to TINY_FLAG_NO_WAIT, then this function may remember pbuf
 *       pointer and return immediately. So, it is responsibility of the caller to make
 *       pbuf data to be available all the time until frame is sent.
 *
 * @param handle - pointer to Tiny data.
 * @param pbuf - pointer to buffer with data to send.
 * @param len - length of the data to send in bytes.
 * @param flags - TINY_FLAG_NO_WAIT or TINY_FLAG_WAIT_FOREVER
 * @return length in bytes of data sent if executed successfully
 *         TINY_NO_ERROR if non-blocking operation is requested and the channel is busy
 *         TINY_ERR_FAILED if writing to the channel failed
 *         TINY_ERR_INVALID_DATA if invalid handle is passed
 *
 * @see TINY_SUCCESS
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_TIMEOUT
 * @see TINY_ERR_FAILED
 * @see TINY_FLAG_NO_WAIT
 * @see TINY_FLAG_WAIT_FOREVER
 * @warning any failed send operation (tiny_send_buffer, tiny_send_end), except timeout cases,
 *          must be terminated with tiny_send_terminate, otherwise the thread can be blocked.
 */
extern int tiny_send_buffer(STinyData *handle, uint8_t * pbuf, int len, uint8_t flags);


/**
 * @brief completes sending of a new frame
 *
 * The function completes sending of a new frame by writing FCS block and frame end marker to
 * communication channel. If the function is executed successfully, then
 * tiny_send_start can be executed again.
 *
 * @param handle - pointer to Tiny data.
 * @param flags - TINY_FLAG_NO_WAIT or TINY_FLAG_WAIT_FOREVER
 * @return TINY_SUCCESS if send frame is completed successfully.
 *         TINY_ERR_TIMEOUT/TINY_NO_ERROR if non-blocking operation is requested and the channel is busy
 *         TINY_ERR_FAILED if writing to the channel failed
 *         TINY_ERR_INVALID_DATA if invalid handle is passed
 *
 * @see TINY_SUCCESS
 * @see TINY_NO_ERROR
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_TIMEOUT
 * @see TINY_ERR_FAILED
 * @see TINY_FLAG_NO_WAIT
 * @see TINY_FLAG_WAIT_FOREVER
 */
extern int tiny_send_end(STinyData *handle, uint8_t flags);

/**
 * @brief terminates send operation
 *
 * This function is to be used, when it is required to reset send state in
 * case of communication error
 *
 * @param handle - pointer to Tiny data.
 */
extern void tiny_send_terminate(STinyData *handle);

/**
 * @brief initiates receiving of a new frame
 *
 * The function initiates receiving of a new frame. It waits for frame start
 * marker in communication channel. If any character different from
 * frame start marker is read from communication channel, it returns
 * TINY_ERR_OUT_OF_SYNC. Just call it once more in this case.
 * If the function is executed successfully, then tiny_read_buffer
 * can be used to read user data from communication channel.
 *
 * @param handle - pointer to Tiny data.
 * @param flags - TINY_FLAG_NO_WAIT, TINY_FLAG_WAIT_FOREVER.
 * @return TINY_SUCCESS if new frame is detected in communication channel.
 *         TINY_NO_ERROR if nothing is awaiting in the communication channel and
 *                       function is in non-blocking mode
 *         TINY_ERR_FAILED if writing to the channel failed
 *         TINY_ERR_INVALID_DATA if invalid handle is passed
 *         TINY_ERR_OUT_OF_SYNC if not valid byte is received
 *
 * @see TINY_SUCCESS
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_NO_ERROR
 * @see TINY_ERR_OUT_OF_SYNC
 * @see TINY_ERR_FAILED
 * @see TINY_FLAG_NO_WAIT
 * @see TINY_FLAG_WAIT_FOREVER
 */
extern int tiny_read_start(STinyData * handle, uint8_t flags);


/**
 * @brief reads frame payload to provided buffer
 *
 * The function reads frame payload to provided buffer. Without TINY_FLAG_READ_ALL
 * the function reads no more byte than len, passed to the function. If provided buffer
 * is filled completely, but there are more bytes to read, then the function returns
 * TINY_ERR_DATA_TOO_LARGE (indicating that tiny_read_buffer must be called
 * once again for next portion of bytes). Otherwise, tiny_read_buffer() returns number of
 * bytes contained in the received frame.
 *
 * If frame payload is too big to fit to provided buffer, but it is required to read
 * whole frame from the communication channel till the end marker, TINY_FLAG_READ_ALL can be specified.
 * In this case, the function will fill provided buffer with frame bytes, and
 * bytes in the end of frame will be lost. The function returns TINY_ERR_DATA_TOO_LARGE, but this
 * means that frame data didn't fit to buffer, and frame is completely received.
 *
 * If function is in non-blocking mode, it may return immediately with
 * TINY_NO_ERROR. In this case a user should call this function later with the
 * same parameters.
 *
 * \note if flags field is set to TINY_FLAG_NO_WAIT, then this function may remember pbuf
 *       pointer and return immediately. So, it is responsibility of the caller to make
 *       pbuf to be available all the time until block of data is received.
 *
 * \note The read data can be considered as valid, only when last buffer is successfully read.
 *       That is tiny_read_buffer function returns length of read bytes (>0), or tiny_read_buffer
 *       function returns TINY_ERR_DATA_TOO_LARGE if TINY_FLAG_READ_ALL flag is specified.
 *
 * @param handle - pointer to Tiny data.
 * @param pbuf - pointer to buffer to read frame data to.
 * @param len - length of the provided buffer in bytes.
 * @param flags - TINY_FLAG_NO_WAIT or TINY_FLAG_WAIT_FOREVER, can be combined with TINY_FLAG_READ_ALL
 * @return length in bytes of data received if executed successfully
 *         TINY_NO_ERROR if non-blocking operation is requested and the channel is busy
 *         TINY_ERR_FAILED if writing to the channel failed
 *         TINY_ERR_INVALID_DATA if invalid handle is passed
 *         TINY_ERR_DATA_TOO_LARGE if all data do not fit in buffer.
 *
 * @see TINY_SUCCESS
 * @see TINY_ERR_INVALID_DATA
 * @see TINY_ERR_FAILED
 * @see TINY_ERR_DATA_TOO_LARGE
 * @see TINY_FLAG_NO_WAIT
 * @see TINY_FLAG_WAIT_FOREVER
 * @see TINY_FLAG_READ_ALL
 * @warning any failed read operation (tiny_read_buffer), except TINY_ERR_DATA_TOO_LARGE case,
 *          must be terminated with tiny_read_terminate.
 */
extern int tiny_read_buffer(STinyData *handle, uint8_t *pbuf, int len, uint8_t flags);


/**
 * @brief terminates read operation
 *
 * This function is to be used, when it is required to reset receive state in
 * case of communication error
 *
 * @param handle - pointer to Tiny data.
 */
extern void tiny_read_terminate(STinyData *handle);


#ifdef CONFIG_ENABLE_STATS
/**
* The function returns statistics per communication connection.
* @param handle - pointer to Tiny data.
* @param stat - pointer of stucture to fill.
* @see TINY_ERR_INVALID_DATA
* @see TINY_NO_ERROR
* @return TINY_ERR_INVALID_DATA, TINY_NO_ERROR.
*/
extern int tiny_get_stat(STinyData *handle, STinyStats *stat);


/**
* The function clears Tiny statistics.
* @param handle - pointer to Tiny data.
* @see TINY_ERR_INVALID_DATA
* @see TINY_NO_ERROR
* @return TINY_ERR_INVALID_DATA, TINY_NO_ERROR.
*/
extern int tiny_clear_stat(STinyData *handle);


/**
* The function sets callback procs for specified Tiny channel.
* callbacks will receive all data being sent or received.
* @param handle - pointer to Tiny data.
* @param read_cb - pointer to callback function.
* @param send_cb - pointer to callback function.
* @return TINY_ERR_INVALID_DATA, TINY_NO_ERROR.
*/
extern int tiny_set_callbacks(STinyData *handle,
               on_frame_cb_t read_cb,
               on_frame_cb_t send_cb);
#endif /* CONFIG_ENABLE_STATS */

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* _TINY_PROTOCOL_H_ */
