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
 @brief Tiny protocol Arduino API

*/

#ifndef _TINY_PROTOCOL_H_
#define _TINY_PROTOCOL_H_

#include "proto/tiny_layer2.h"

#ifdef ARDUINO
#   include <HardwareSerial.h>
#else
#   include <string.h>
#endif

namespace Tiny {

class Packet;

/**
 *  Proto class incapsulates Protocol functionality.
 *  Remember that you may use always C-style API functions
 *  instead C++. Please refer to documentation.
 */
class Proto
{
public:
    inline Proto() { m_uidEnabled = false; }

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
         begin([](const uint8_t *b, int s)->int { return Serial.write(b, s); },
               [](uint8_t *b, int s)->int { return Serial.readBytes(b, s); });
    }

#if HAVE_HWSERIAL1
    /**
     * Initializes protocol internal variables and redirects
     * communication through Arduino Serial1 connection (Serial1).
     * @return None
     */
    inline void beginToSerial1()
    {
         begin([](const uint8_t *b, int s)->int { return Serial1.write(b, s); },
               [](uint8_t *b, int s)->int { return Serial1.readBytes(b, s); });
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
         begin([](const uint8_t *b, int s)->int { return Serial2.write(b, s); },
               [](uint8_t *b, int s)->int { return Serial2.readBytes(b, s); });
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
         begin([](const uint8_t *b, int s)->int { return Serial3.write(b, s); },
               [](uint8_t *b, int s)->int { return Serial3.readBytes(b, s); });
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


/**
 * Describes packet entity and provides API methods to
 * manipulate the packet.
 */
class Packet
{
public:
    /**
     * Creates packet object.
     * @param buf - pointer to the buffer to store packet data
     * @param size - size of the buffer to hold packet data
     * @note passed buffer must exist all lifecycle of the Packet object.
     */
    Packet(char *buf, size_t size)     { m_len = 0; m_size = size; m_buf = (uint8_t*)buf; }

    /**
     * Clears Packet state. Buffer and its size are preserved.
     */
    inline void clear  ()              { m_len = 0; m_p = 0; }

    /**
     * Puts next byte to the packet. For example, after calling this method
     * twice: put(5), put(10), - the Packet will contain 5,10.
     * @param byte - data byte to put.
     */
    inline void put    (uint8_t byte)  { m_buf[m_len++] = byte; }

    /**
     * Puts next char to the packet. For example, after calling this method
     * twice: put('a'), put('c'), - the Packet will contain 'ac'.
     * @param chr - character to put.
     */
    inline void put    (char chr)      { put((uint8_t)chr); }

    /**
     * Puts next 16-bit unsigned integer to the packet.
     * @param data - data to put.
     */
    inline void put    (uint16_t data) { m_buf[m_len++] = data & 0x00FF;
                                         m_buf[m_len++] = data >> 8; }

    /**
     * Puts next 32-bit unsigned integer to the packet.
     * @param data - data to put.
     */
    inline void put    (uint32_t data) { put((uint16_t)(data & 0x0000FFFF));
                                         put((uint16_t)(data >> 16)); }

    /**
     * Puts next 16-bit signed integer to the packet.
     * @param data - data to put.
     */
    inline void put    (int16_t  data) { put((uint16_t)data); }

    /**
     * Puts next null-terminated string to the packet.
     * @param str - string to put.
     */
    inline void put    (const char *str){ strncpy((char *)&m_buf[m_len], str, m_size - m_len);
                                          m_len += strlen(str);
                                          m_buf[m_len++] = 0; }

    /**
     * Adds data from packet to the new packet being built.
     * @param pkt - reference to the Packet to add.
     */
    inline void put    (const Packet &pkt){ memcpy(&m_buf[m_len], pkt.m_buf, pkt.m_len); m_len += pkt.m_len; }

    /**
     * Puts uid to the packet.
     * @warning uid is sent only if this functionality is enabled in the Proto.
     * @see Proto::enableUid
     * @see Proto::disableUid
     * @param uid - 16-bit number to place to the packet.
     */
    inline void putUid (uint16_t uid)  { m_uid = uid; }

    /**
     * Reads next byte from the packet.
     * @return byte from the packet.
     */
    inline uint8_t getByte   ()        { return m_buf[m_p++]; }

    /**
     * Reads next character from the packet.
     * @return character from the packet.
     */
    inline char getChar      ()        { return (char)Packet::getByte(); }

    /**
     * Reads next unsigned 16-bit integer from the packet.
     * @return unsigned 16-bit integer.
     */
    inline uint16_t getUint16()        { uint16_t t = m_buf[m_p++]; return t | ((uint16_t)m_buf[m_p++] << 8); }

    /**
     * Reads next signed 16-bit integer from the packet.
     * @return signed 16-bit integer.
     */
    inline int16_t getInt16  ()        { return (int16_t)(getUint16()); }

    /**
     * Reads next unsigned 32-bit integer from the packet.
     * @return unsigned 32-bit integer.
     */
    inline uint32_t getUint32()        { return getUint16() | ((uint32_t)getUint16())<<16; }

    /**
     * Reads zero-terminated string from the packet.
     * @return zero-terminated string.
     */
    inline char* getString   ()        { char *p = (char *)&m_buf[m_p]; m_p += strlen((char*)m_buf) + 1; return p; }

    /**
     * Returns 16-bit identificator of the packet.
     * @warning uid is valid only if uid functionality is enabled in the Proto
     * @see Proto::enableUid
     * @see Proto::disableUid
     * @return 16-bit uid.
     */
    inline uint16_t getUid   ()        { return m_uid; }

    /**
     * Returns size of payload data in the received packet.
     * @return size of payload data.
     */
    inline size_t size       ()        { return m_len; }

    /**
     * Returns maximum size of packet buffer.
     * @return max size of packet buffer.
     */
    inline size_t maxSize    ()        { return m_size; }

    /**
     * Returns size of payload data in the received packet.
     * @return size of payload data.
     */
    inline char *data        ()        { return (char*)m_buf; }

    /**
     * You may refer to Packet payload data directly by using operator []
     */
    uint8_t &operator[]      (size_t idx) { return m_buf[idx]; }

    /**
     * Assign operator = puts next char to the packet. Several
     * assign operators put one by one several chars.
     */
    Packet &operator=        (char chr){ put(chr); return *this; }

private:
    friend class        Proto;

    uint8_t*            m_buf;
    uint16_t            m_uid;
    uint8_t             m_size;
    uint8_t             m_len;
    uint8_t             m_p;
};


} // Tiny namespace

#endif

