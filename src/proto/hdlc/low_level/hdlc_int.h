/*
    Copyright 2019-2022 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

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

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/
#pragma once

#include "hal/tiny_types.h"
#include "proto/crc/tiny_crc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @ingroup HDLC_LOW_LEVEL_API
 * @{
 *
 * @brief low level HDLC protocol function - only framing
 *
 * @details this group implements low level HDLC functions, which implement
 *          framing only according to RFC 1662: 0x7E, 0x7D, 0x20 (ISO Standard 3309-1979).
 */

/**
 * Macro calculating minimum buffer size required for specific packet size in bytes
 */
#define HDLC_MIN_BUF_SIZE(mtu, crc) (sizeof(hdlc_ll_data_t) + (int)(crc) / 8 + (mtu) + TINY_ALIGN_STRUCT_VALUE - 1)

    /**
     * Structure describes configuration of lowest HDLC level
     * Initialize this structure by 0 before passing to hdlc_ll_init()
     * function.
     */
    typedef struct hdlc_ll_data_t
    {
        /**
         * User-defined callback, which is called when new packet arrives from hw
         * channel. The context of this callback is context, where hdlc_ll_run_rx() is
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
         * channel. The context of this callback is context, where hdlc_ll_run_tx() is
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
         * size of hdlc buffer
         */
        int rx_buf_size;

        /**
         * crc field type to use on hdlc level.
         * If HDLC_CRC_DEFAULT is passed, crc type will be selected automatically (depending on library configuration),
         * but HDLC_CRC_16 has higher priority.
         */
        hdlc_crc_t crc_type;

        /** User data, which will be passed to user-defined callback as first argument */
        void *user_data;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
        /** Parameters in DOXYGEN_SHOULD_SKIP_THIS section should not be modified by a user */
        struct
        {
            int (*state)(hdlc_ll_handle_t handle, const uint8_t *data, int len);
            uint8_t *data;
            uint8_t escape;
        } rx;
        struct
        {
            int (*state)(hdlc_ll_handle_t handle);
            uint8_t *out_buffer;
            int out_buffer_len;
            const uint8_t *origin_data;
            const uint8_t *data;
            int len;
            crc_t crc;
            uint8_t escape;
        } tx;
#endif
    } hdlc_ll_data_t;

    /**
     * @}
     */

#ifdef __cplusplus
}
#endif
