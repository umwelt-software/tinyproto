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
 This is Tiny light protocol implementation for microcontrollers

 @file
 @brief Tiny light protocol Arduino API
*/

#ifndef _TINY_LIGHT_PROTOCOL_H_
#define _TINY_LIGHT_PROTOCOL_H_

#include "TinyPacket.h"
#include "proto/tiny_light.h"

#ifdef ARDUINO
#   include <HardwareSerial.h>
#else
#   include <string.h>
#endif

namespace Tiny {

/**
 *  ProtoLight class incapsulates Protocol functionality.
 *  Remember that you may use always C-style API functions
 *  instead C++. Please refer to documentation.
 */
class ProtoLight
{
public:
    inline ProtoLight(): m_data{0} { }

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
    void beginToSerial();
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
     * Reads data block from communication channel.
     * @param buf - buffer to place data read from communication channel
     * @param size - maximum size of the buffer in bytes.
     * @return negative value in case of error
     *         zero if nothing is read
     *         positive - number of bytes read from the channel
     */
    int  read           (char* buf, int size);

    /**
     * Sends packet over communication channel.
     * @param pkt - Packet to send
     * @see Packet
     * @return negative value in case of error
     *         zero if nothing is sent
     *         positive - Packet is successfully sent
     */
    int  write          (Packet &pkt);

    /**
     * Reads packet from communication channel.
     * @param pkt - Packet object to put data to
     * @see Packet
     * @return negative value in case of error
     *         zero if nothing is read
     *         positive - Packet is successfully received
     */
    int  read           (Packet &pkt);

private:
    STinyLightData       m_data;
};


} // Tiny namespace


#endif
