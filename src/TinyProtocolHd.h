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
 This is Tiny protocol implementation for microcontrollers

 @file
 @brief Tiny protocol Arduino API

*/

#ifndef _TINY_PROTOCOL_H_
#define _TINY_PROTOCOL_H_

#include "TinyPacket.h"
#include "proto/half_duplex/tiny_hd.h"

#ifdef ARDUINO
#   include <HardwareSerial.h>
#else
#   include <string.h>
#endif

namespace Tiny {

/**
 * @ingroup HALF_DUPLEX_API
 * @{
 */

/**
 *  ProtoHd class incapsulates Half Duplex Protocol functionality.
 *  Half Duplex version of the Protocol allows to send messages with
 *  confirmation.
 *  Remember that you may use always C-style API functions
 *  instead C++. Please refer to documentation.
 */
class ProtoHd
{
private:
    /** The variable contain protocol state */
    STinyHdData         m_data;

public:
    /**
     * Initializes ProtoHd object
     * @param buffer - buffer to store the frames being received.
     * @param bufferSize - size of the buffer
     * @param onReceive - callback to call when the frame is received completely.
     */
    inline ProtoHd(void * buffer,
                   int    bufferSize,
                   void (*onReceive)(uint8_t *buf, int len))
           :m_data{}
    {
        m_buffer      = buffer;
        m_bufferSize  = bufferSize;
        m_onReceive   = onReceive;
    }

    /**
     * Initializes protocol internal variables.
     * If you need to switch communication with other destination
     * point, you can call this method one again after calling end().
     * @param writecb - write function to some physical channel
     * @param readcb  - read function from some physical channel
     * @return None
     */
    void begin          (write_block_cb_t writecb,
                         read_block_cb_t readcb);

#ifdef ARDUINO
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial connection (Serial).
     * @return None
     */
    inline void beginToSerial()
    {
         begin([](void *p, const void *b, int s)->int { return Serial.write((const uint8_t *)b, s); },
               [](void *p, void *b, int s)->int { return Serial.readBytes((uint8_t *)b, s); });
    }

#ifdef HAVE_HWSERIAL1
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial1 connection (Serial1).
     * @return None
     */
    inline void beginToSerial1()
    {
         begin([](void *p, const void *b, int s)->int { return Serial1.write((const uint8_t *)b, s); },
               [](void *p, void *b, int s)->int { return Serial1.readBytes((uint8_t *)b, s); });
    }
#endif

#ifdef HAVE_HWSERIAL2
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial2 connection (Serial2).
     * @return None
     */
    inline void beginToSerial2()
    {
         begin([](void *p, const void *b, int s)->int { return Serial2.write((const uint8_t *)b, s); },
               [](void *p, void *b, int s)->int { return Serial2.readBytes((uint8_t *)b, s); });
    }
#endif

#ifdef HAVE_HWSERIAL3
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial3 connection (Serial3).
     * @return None
     */
    inline void beginToSerial3()
    {
         begin([](void *p, const void *b, int s)->int { return Serial3.write((const uint8_t *)b, s); },
               [](void *p, void *b, int s)->int { return Serial3.readBytes((uint8_t *)b, s); });
    }
#endif


#ifdef HAVE_SERIALUSB
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial1 connection (SerialUSB).
     * @return None
     */
    inline void beginToSerialUSB()
    {
         begin([](void *p, const void *b, int s)->int { return SerialUSB.write((const char *)b, s); },
               [](void *p, void *b, int s)->int { return SerialUSB.readBytes((char *)b, s); });
    }
#endif

#endif

    /**
     * Resets protocol state.
     */
    void end            ();

    /**
     * Sends data block over communication channel.
     * @param buf - data to send
     * @param size - length of the data in bytes
     * @return negative value in case of error
     *         zero if nothing is sent
     *         positive - should be equal to size parameter
     */
    int  write          (char* buf, int size);

    /**
     * Sends packet over communication channel.
     * @param pkt - Packet to send
     * @see Packet
     * @return negative value in case of error
     *         zero if nothing is sent
     *         positive - Packet is successfully sent
     */
    int  write          (IPacket &pkt);

    /**
     * Checks communcation channel for incoming messages.
     * @return negative value in case of error
     *         zero if nothing is read
     *         positive - Packet is successfully received
     * @remark if Packet is receive during run execution
     *         callback is called.
     */
    int  run            ();

    /**
     * Disable CRC field in the protocol.
     * If CRC field is OFF, then the frame looks like this:
     * 0x7E databytes 0x7E.
     */
    void disableCrc     ();

    /**
     * Enables CRC 8-bit field in the protocol. This field
     * contains sum of all data bytes in the packet.
     * 8-bit field is supported by Nano version of Tiny library.
     * @return true if successful
     *         false in case of error.
     */
    bool enableCheckSum ();

    /**
     * Enables CRC 16-bit field in the protocol. This field
     * contains FCS 16-bit CCITT like defined in RFC 1662.
     * 16-bit field is not supported by Nano version of Tiny library.
     * @return true if successful
     *         false in case of error.
     */
    bool enableCrc16    ();

    /**
     * Enables CRC 32-bit field in the protocol. This field
     * contains FCS 32-bit CCITT like defined in RFC 1662.
     * 32-bit field is not supported by Nano version of
     * Tiny library.
     * @return true if successful
     *         false in case of error.
     */
    bool enableCrc32    ();

private:
    /** buffer to receive data to */
    void               *m_buffer;
    hdlc_crc_t          m_crc = HDLC_CRC_DEFAULT;
    /** max buffer size */
    int                 m_bufferSize;
    /** Callback, when new frame is received */
    void              (*m_onReceive)(uint8_t *buf, int len);
    /** Internal function */
    static void         onReceiveInternal(void *handle, uint16_t uid, uint8_t *pdata, int size);

};

/**
 * @}
 */

} // Tiny namespace

#endif

