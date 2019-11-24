#pragma once

#include "proto/hal/tiny_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup HDLC_API Tiny HDLC protocol API functions
 * @{
 *
 * @brief low level HDLC protocol function - only framing
 *
 * @details this group implements low level HDLC functions, which implement
 *          framing only according to RFC 1662: 0x7E, 0x7D, 0x20 (ISO Standard 3309-1979).
 */

/// \cond
#ifdef CONFIG_ENABLE_FCS32
    typedef uint32_t crc_t;
#else
    typedef uint16_t crc_t;
#endif
/// \endcond

/**
 * HDLC CRC options, available
 */
typedef enum
{
    HDLC_CRC_DEFAULT = 0, ///< If default is specified HDLC will auto select CRC option
    HDLC_CRC_8 = 8,       ///< Simple sum of all bytes in user payload
    HDLC_CRC_16 = 16,     ///< CCITT-16
    HDLC_CRC_32 = 32,     ///< CCITT-32
    HDLC_CRC_OFF = 0xFF,  ///< Disable CRC field
} hdlc_crc_t;

/**
 * Structure describes configuration of lowest HDLC level
 * Initialize this structure by 0 before passing to hdlc_init()
 * function.
 */
typedef struct _hdlc_handle_t
{
    /**
     * Send bytes callback user-defined function. This callback
     * must implement physical sending of bytes hw channel.
     * @param user_data user-defined data
     * @param data buffer with data to send over hw channel
     * @param len size of data in bytes to send.
     * @return user callback must return negative value in case of error or
     *         0 if hw device is busy, or positive number - number of bytes
     *         sent.
     */
    write_block_cb_t send_tx;

    /**
     * User-defined callback, which is called when new packet arrives from hw
     * channel. The context of this callback is context, where hdlc_run_rx() is
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
     * channel. The context of this callback is context, where hdlc_run_tx() is
     * called from.
     * @param user_data user-defined data
     * @param data pointer to sent data
     * @param len size of sent data in bytes
     * @return user callback must return negative value in case of error
     *         or 0 value if packet is successfully processed.
     */
    int (*on_frame_sent)(void *user_data, const void *data, int len);

    /**
     * Buffer to be used by hdlc level to receive data to
     */
    void *rx_buf;

    /**
     * size of rx buffer
     */
    int rx_buf_size;

    /**
     * crc field type to use on hdlc level.
     * If HDLC_CRC_DEFAULT is passed, crc type will be selected automatically (depending on library configuration),
     * but HDLC_CRC_16 has higher priority.
     */
    hdlc_crc_t crc_type;

    /**
     * Set this to true, if you want to implements TX data transmission in separate
     * thread from the threads, which call hdlc_send().
     */
    bool multithread_mode;

    /** User data, which will be passed to user-defined callback as first argument */
    void *user_data;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /** Parameters in DOXYGEN_SHOULD_SKIP_THIS section should not be modified by a user */
    tiny_mutex_t send_mutex;
    tiny_events_t events;
    struct
    {
        uint8_t *data;
        int (*state)( struct _hdlc_handle_t *handle, const uint8_t *data, int len );
        uint8_t escape;
    } rx;
    struct
    {
        const uint8_t *origin_data;
        const uint8_t *data;
        int len;
        crc_t crc;
        uint8_t escape;
        int (*state)( struct _hdlc_handle_t *handle );
    } tx;
#endif
} hdlc_struct_t, *hdlc_handle_t; ///< hdlc handle

//------------------------ GENERIC FUNCIONS ------------------------------

/**
 * Initializes hdlc level and returns hdlc handle or NULL in case of error.
 *
 * @param hdlc_info pointer to hdlc_struct_t structure, which defines user-specific configuration
 * @return handle to hdlc instance or NULL.
 * @warning hdlc_info structure passed to the function must be allocated all the time.
 */
hdlc_handle_t hdlc_init( hdlc_struct_t *hdlc_info );

/**
 * Shutdowns all hdlc activity
 *
 * @param handle handle to hdlc instance
 */
int hdlc_close( hdlc_handle_t handle );

/**
 * Resets hdlc state. Use this function, if hw error happened on tx or rx
 * line, and this requires hardware change, and cancelling current operation.
 *
 * @param handle handle to hdlc instance
 */
void hdlc_reset( hdlc_handle_t handle );

//------------------------ RX FUNCIONS ------------------------------

/**
 * Processes incoming data. Implementation of reading data from hw is user
 * responsibility. If hdlc_run_rx() returns value less than size of data
 * passed to the function, then hdlc_run_rx() must be called later second
 * time with the pointer to and size of not processed bytes.
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
 */
int hdlc_run_rx( hdlc_handle_t handle, const void *data, int len, int *error );

/**
 * Runs rx cycle until full frame received.
 *
 * @param handle handle to hdlc instance
 * @param readcb callback to read bytes from channel function
 * @param user_data user data to pass to readcb callback function
 * @param timeout timeout in milliseconds to wait for new frame.
 * @return
 *   - TINY_SUCCESS if operation completed successfully
 *   - TINY_ERR_FAILED if generic failure happened
 *   - TINY_ERR_TIMEOUT if operation cannot be completed in specified time.
 */
int hdlc_run_rx_until_read( hdlc_handle_t handle, read_block_cb_t readcb, void *user_data, uint16_t timeout );

/**
 * Sets new RX buffer for hdlc protocol. This function is not thread-safe.
 *
 * @param handle handle to hdlc instance
 * @param data pointer to rx buffer
 * @param size size of rx buffer in bytes
 */
void hdlc_set_rx_buffer( hdlc_handle_t handle, void *data, int size);

//------------------------ TX FUNCIONS ------------------------------

/**
 * Runs transmission at hdlc level. If there is frame to send, or
 * send is in progress, this function will call send_tx() callback
 * multiple times. If send_tx() callback reports 0, that means that
 * hw device is busy and in this case hdlc_run_tx() will return immediately.
 *
 * @warning this function must be called from one thread only.
 *
 * @param handle handle to hdlc instance
 * @return negative value in case of error
 *         0 if hw is busy, or no data is waiting for sending
 *         positive number of bytes passed to hw channel.
 */
int hdlc_run_tx( hdlc_handle_t handle );

/**
 * Puts next frame for sending. This function is thread-safe.
 * You may call it from parallel threads.
 *
 * If multithread_mode is specificed for hdlc protocol, then
 * hdlc_send() function will wait for specified timeout until
 * some tx thread, implemented by application, completes sending
 * of the frame. If timeout happens, but
 * the frame is not sent still, hdlc level rejects sending of the frame.
 * In this case the frame will be set partially, causing RX errors on
 * other side. Please use reasonable timeout.
 *
 * If multithread_mode is disabled for hdlc protocol, then
 * hdlc_send() function will start frame sending immediately by
 * itself if TX line is not busy. hdlc_send() will
 * block until frame is sent or timeout. If timeout happens, but
 * the frame is not sent still, hdlc level rejects sending of the frame.
 * In this case the frame will be set partially, causing RX errors on
 * other side. Please use reasonable timeout.
 *
 * If timeout is specified as 0, hdlc_send() function will not
 * wait or perform send operation, but only pass data pointer to
 * hdlc state machine. In this case, some other thread needs to
 * or in the same thread you need to send data using hdlc_run_tx().
 *
 * @param handle handle to hdlc instance
 * @param data pointer to new data to send (can be NULL is you need to retry sending)
 * @param len size of data to send in bytes
 * @param timeout time in milliseconds to wait for data to be sent
 * @return TINY_ERR_FAILED if generic error happens
 *         TINY_ERR_BUSY if TX queue is busy with another frame.
 *         TINY_ERR_TIMEOUT if send operation cannot be completed in specified time.
 *         TINY_ERR_INVALID_DATA if len is zero.
 *         TINY_SUCCESS if data is successfully sent
 * @warning buffer with data must be available all the time until
 *          data are actually sent to tx hw channel. That is if you use
 *          zero timeout, you need to use callback to understand, when buffer
 *          is not longer needed at hdlc level.
 * @note TINY_ERR_BUSY and TINY_ERR_INVALID_DATA refer to putting new frame to TX
 *       hdlc queue.
 */
int hdlc_send( hdlc_handle_t handle, const void *data, int len, uint32_t timeout );

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

