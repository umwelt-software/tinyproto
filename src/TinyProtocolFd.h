/*
    Copyright 2019-2021 (C) Alexey Dynda

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
 @brief Tiny protocol Arduino API

*/
#pragma once

#include "TinyPacket.h"
#include "proto/fd/tiny_fd.h"

#ifdef ARDUINO
#include <HardwareSerial.h>
#else
#include <string.h>
#include <stdlib.h>
#endif

namespace tinyproto
{

/**
 * @ingroup FULL_DUPLEX_API
 * @{
 */

/**
 *  IFd class incapsulates Full Duplex Protocol functionality.
 *  Full Duplex version of the Protocol allows to send messages with
 *  confirmation.
 *  Remember that you may use always C-style API functions
 *  instead C++. Please refer to documentation.
 */
class IFd
{
public:
    friend class FdD;
    /**
     * Initializes IFd object
     * @param buffer - buffer to store the frames being received.
     * @param bufferSize - size of the buffer
     */
    IFd(void *buffer, int bufferSize)
        : m_buffer((uint8_t *)buffer)
        , m_bufferSize(bufferSize)
    {
    }

    virtual ~IFd() = default;

    /**
     * Initializes protocol internal variables.
     * If you need to switch communication with other destination
     * point, you can call this method one again after calling end().
     * Use this method if you want to control write and read data by yourself
     * @return None
     */
    void begin();

    /**
     * Resets protocol state.
     */
    void end();

    /**
     * Sends data block over communication channel.
     * @param buf - data to send
     * @param size - length of the data in bytes
     * @return negative value in case of error
     *         zero if nothing is sent
     *         positive - should be equal to size parameter
     */
    int write(const char *buf, int size);

    /**
     * Sends packet over communication channel.
     * @param pkt - Packet to send
     * @see Packet
     * @return negative value in case of error
     *         zero if nothing is sent
     *         positive - Packet is successfully sent
     */
    int write(const IPacket &pkt);

    /**
     * Processes incoming rx data, specified by a user.
     * @param data pointer to the buffer with incoming data
     * @param len size of the buffer in bytes
     * @return TINY_SUCCESS
     * @remark if Packet is receive during run execution
     *         callback is called.
     */
    int run_rx(const void *data, int len);

    /**
     * Read data from communication channel using read_func and
     * parses bytes to find hdlc messages.
     * @param read_func function to read data from communication channel
     * @return TINY_SUCCESS
     */
    int run_rx(read_block_cb_t read_func);

    /**
     * Attempts to send out data via write_func function.
     * @param write_func pointer to function for sending bytes to the channel
     * @return TINY_SUCCESS
     */
    int run_tx(write_block_cb_t write_func);

    /**
     * Force protocol to generate tx data for you.
     * In this case you will need to send data to remote side by yourself.
     * @param data buffer to fill with tx data
     * @param max_size maximum size of the buffer
     * @return number of bytes written to buffer
     */
    int run_tx(void *data, int max_size);

    /**
     * Disable CRC field in the protocol.
     * If CRC field is OFF, then the frame looks like this:
     * 0x7E databytes 0x7E.
     */
    void disableCrc();

    /**
     * Enables CRC by specified bit-size.
     * 8-bit is supported by Nano version of Tiny library.
     * @param crc crc type
     */
    void enableCrc(hdlc_crc_t crc);

    /**
     * Enables CRC 8-bit field in the protocol. This field
     * contains sum of all data bytes in the packet.
     * 8-bit field is supported by Nano version of Tiny library.
     * @return true if successful
     *         false in case of error.
     */
    bool enableCheckSum();

    /**
     * Enables CRC 16-bit field in the protocol. This field
     * contains FCS 16-bit CCITT like defined in RFC 1662.
     * 16-bit field is not supported by Nano version of Tiny library.
     * @return true if successful
     *         false in case of error.
     */
    bool enableCrc16();

    /**
     * Enables CRC 32-bit field in the protocol. This field
     * contains FCS 32-bit CCITT like defined in RFC 1662.
     * 32-bit field is not supported by Nano version of
     * Tiny library.
     * @return true if successful
     *         false in case of error.
     */
    bool enableCrc32();

    /**
     * Sets receive callback for incoming messages
     * @param on_receive user callback to process incoming messages. The processing must be non-blocking
     */
    void setReceiveCallback(void (*on_receive)(void *userData,IPacket &pkt) = nullptr)
    {
        m_onReceive = on_receive;
    };

    /**
     * Sets send callback for outgoing messages
     * @param on_send user callback to process outgoing messages. The processing must be non-blocking
     */
    void setSendCallback(void (*on_send)(void *userData, IPacket &pkt) = nullptr)
    {
        m_onSend = on_send;
    };

    /**
     * Sets desired window size. Use this function only before begin() call.
     * window size is number of frames, which confirmation may be deferred for.
     * @param window window size, valid between 1 - 7 inclusively
     * @warning if you use smallest window size, this can reduce throughput of the channel.
     */
    void setWindowSize(uint8_t window)
    {
        m_window = window;
    }

    /**
     * Sets send timeout in milliseconds.
     * @param timeout timeout in milliseconds,
     */
    void setSendTimeout(uint16_t timeout)
    {
        m_sendTimeout = timeout;
    }

    /**
     * Sets user data to pass to callbacks
     * @param userData user data to pass to callback
     */
    void setUserData(void *userData)
    {
        m_userData = userData;
    }

protected:
    /**
     * Method called by hdlc protocol upon receiving new frame.
     * Can be redefined in derived classes.
     * @param pdata pointer to received data
     * @param size size of received payload in bytes
     */
    virtual void onReceive(uint8_t *pdata, int size)
    {
        IPacket pkt((char *)pdata, size);
        pkt.m_len = size;
        if ( m_onReceive )
            m_onReceive(m_userData, pkt);
    }

    /**
     * Method called by hdlc protocol upon sending next frame.
     * Can be redefined in derived classes.
     * @param pdata pointer to sent data
     * @param size size of sent payload in bytes
     */
    virtual void onSend(uint8_t *pdata, int size)
    {
        IPacket pkt((char *)pdata, size);
        pkt.m_len = size;
        if ( m_onSend )
            m_onSend(m_userData, pkt);
    }

private:
    /** The variable contain protocol state */
    tiny_fd_handle_t m_handle = nullptr;

    /** buffer to receive data to */
    uint8_t *m_buffer = nullptr;

    hdlc_crc_t m_crc = HDLC_CRC_DEFAULT;

    /** max buffer size */
    int m_bufferSize = 0;

    /** Use 0-value timeout for small controllers as all operations should be non-blocking */
    uint16_t m_sendTimeout = 0;

    /** Limit window to only 3 frames for small controllers by default */
    uint8_t m_window = 3;

    /** Callback, when new frame is received */
    void (*m_onReceive)(void *userData, IPacket &pkt) = nullptr;

    /** Callback, when new frame is sent */
    void (*m_onSend)(void *userData, IPacket &pkt) = nullptr;

    void *m_userData = nullptr;

    /** Internal function */
    static void onReceiveInternal(void *handle, uint8_t *pdata, int size);

    /** Internal function */
    static void onSendInternal(void *handle, uint8_t *pdata, int size);
};

/**
 * This is class, which allocates buffers statically. Use it for systems with low resources.
 */
template <int S> class Fd: public IFd
{
public:
    Fd()
        : IFd(m_data, S)
    {
    }

private:
    TINY_ALIGNED_PTR uint8_t m_data[S]{};
};

/**
 * This is special class for Full duplex protocol, which allocates buffers dynamically.
 * We need to have separate class for this, as on small microcontrollers dynamic allocation
 * in basic class increases flash consumption, even if dynamic memory is not used.
 */
class FdD: public IFd
{
public:
    /**
     * Creates instance of Full duplex protocol with dynamically allocated buffer.
     * Use this class only on powerful microcontrollers.
     */
    explicit FdD(int size)
        : IFd(reinterpret_cast<uint8_t *>(new uintptr_t[(size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t)]), size)
    {
    }

    ~FdD()
    {
        delete[] m_buffer;
    }

private:
};

/**
 * @}
 */

} // namespace tinyproto
