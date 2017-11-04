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

/**
 This is Tiny protocol implementation for microcontrollers

 @file
 @brief Tiny protocol Arduino API

*/

#ifndef _TINY_PROTOCOL_H_
#define _TINY_PROTOCOL_H_

#include "TinyPacket.h"
#include "proto/tiny_layer2.h"

#ifdef ARDUINO
#   include <HardwareSerial.h>
#else
#   include <string.h>
#endif

namespace Tiny {

/**
 *  Proto class incapsulates Protocol functionality.
 *  Remember that you may use always C-style API functions
 *  instead C++. Please refer to documentation.
 */
class Proto
{
public:
    inline Proto(): m_data{0} { m_uidEnabled = false; }

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
         begin([](void *p, const uint8_t *b, int s)->int { return Serial.write(b, s); },
               [](void *p, uint8_t *b, int s)->int { return Serial.readBytes(b, s); });
    }

#if HAVE_HWSERIAL1
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial1 connection (Serial1).
     * @return None
     */
    inline void beginToSerial1()
    {
         begin([](void *p, const uint8_t *b, int s)->int { return Serial1.write(b, s); },
               [](void *p, uint8_t *b, int s)->int { return Serial1.readBytes(b, s); });
    }
#endif

#if HAVE_HWSERIAL2
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial2 connection (Serial2).
     * @return None
     */
    inline void beginToSerial2()
    {
         begin([](void *p, const uint8_t *b, int s)->int { return Serial2.write(b, s); },
               [](void *p, uint8_t *b, int s)->int { return Serial2.readBytes(b, s); });
    }
#endif

#if HAVE_HWSERIAL3
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial3 connection (Serial3).
     * @return None
     */
    inline void beginToSerial3()
    {
         begin([](void *p, const uint8_t *b, int s)->int { return Serial3.write(b, s); },
               [](void *p, uint8_t *b, int s)->int { return Serial3.readBytes(b, s); });
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
     * @param flags - flags. @ref FLAGS_GROUP
     * @return negative value in case of error
     *         zero if nothing is sent
     *         positive - should be equal to size parameter
     */
    int  write          (char* buf, int size, uint8_t flags = TINY_FLAG_WAIT_FOREVER);

    /**
     * Reads data block from communication channel.
     * @param buf - buffer to place data read from communication channel
     * @param size - maximum size of the buffer in bytes.
     * @param flags - @ref FLAGS_GROUP
     * @return negative value in case of error
     *         zero if nothing is read
     *         positive - number of bytes read from the channel
     */
    int  read           (char* buf, int size, uint8_t flags = TINY_FLAG_WAIT_FOREVER);

    /**
     * Sends packet over communication channel.
     * @param pkt - Packet to send
     * @param flags - @ref FLAGS_GROUP
     * @see Packet
     * @return negative value in case of error
     *         zero if nothing is sent
     *         positive - Packet is successfully sent
     */
    int  write          (Packet &pkt, uint8_t flags = TINY_FLAG_WAIT_FOREVER);

    /**
     * Reads packet from communication channel.
     * @param pkt - Packet object to put data to
     * @param flags - @ref FLAGS_GROUP
     * @see Packet
     * @return negative value in case of error
     *         zero if nothing is read
     *         positive - Packet is successfully received
     */
    int  read           (Packet &pkt, uint8_t flags = TINY_FLAG_WAIT_FOREVER);

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

    /**
     * Enables Uid field in the protocol. If enabled this 16-bit field
     * with packet identifier is added before each payload data.
     * Frame with uid: 0x7E 16-bituid payload 0x7E
     * Frame without uid: 0x7E payload 0x7E
     */
    inline void enableUid() { m_uidEnabled = true; }

    /**
     * Disables Uid field in the protocol. If enabled this 16-bit field
     * with packet identifier is added before each payload data. 
     * Frame with uid: 0x7E 16-bituid payload 0x7E
     * Frame without uid: 0x7E payload 0x7E
     */
    inline void disableUid(){ m_uidEnabled = false; }

private:
    STinyData           m_data;
    uint8_t             m_uidEnabled;
};


} // Tiny namespace

#endif

