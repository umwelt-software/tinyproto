/*
    Copyright 2017-2019 (C) Alexey Dynda

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


#include <functional>
#include <CppUTest/TestHarness.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "proto/hal/tiny_types.h"
#include "proto/hal/tiny_list.h"
#include "proto/hal/tiny_debug.h"
#include "proto/crc/crc.h"


TEST_GROUP(HAL)
{
    void setup()
    {
        // ...
    }

    void teardown()
    {
        // ...
    }
};

TEST(HAL, mutex)
{
    tiny_mutex_t mutex;
    tiny_mutex_create( &mutex );
    try
    {
        tiny_mutex_lock( &mutex );
        CHECK_EQUAL( 0, tiny_mutex_try_lock( &mutex ) );
        tiny_mutex_unlock( &mutex );
        CHECK_EQUAL( 1, tiny_mutex_try_lock( &mutex ) );
        CHECK_EQUAL( 0, tiny_mutex_try_lock( &mutex ) );
        tiny_mutex_unlock( &mutex );
        tiny_mutex_destroy( &mutex );
    }
    catch(...)
    {
        tiny_mutex_destroy( &mutex );
        throw;
    }
}

extern "C" void tiny_list_init(void);

TEST(HAL, list)
{
    list_element el1{};
    list_element el2{};
    list_element el3{};
    list_element *list = nullptr;
    tiny_list_init();
    tiny_list_add( &list, &el1 );
    CHECK_EQUAL( &el1, list );
    tiny_list_add( &list, &el2 );
    CHECK_EQUAL( &el2, list );
    tiny_list_add( &list, &el3 );
    CHECK_EQUAL( &el3, list );
    static int cnt = 0;
    tiny_list_enumerate( list, [](list_element *, uint16_t data)->uint8_t { cnt += data; return 1; }, 2 );
    CHECK_EQUAL( 6, cnt );
    tiny_list_remove( &list, &el2 );
    CHECK_EQUAL( &el3, list );
    tiny_list_remove( &list, &el3 );
    CHECK_EQUAL( &el1, list );
    tiny_list_clear( &list );
    CHECK_EQUAL( (list_element *)nullptr, list );
}

TEST(HAL,loglevel)
{
    tiny_log_level( 5 );
    CHECK_EQUAL( 5, g_tiny_log_level );
    tiny_log_level( 0 );
    CHECK_EQUAL( 0, g_tiny_log_level );
}

TEST(HAL,crc)
{
    uint8_t buf[256];
    for (unsigned int i=0; i<256; i++) buf[i] = i;
    uint8_t crc8_num = chksum( INITCHECKSUM, buf, sizeof(buf) );
    uint16_t crc16_num = crc16( PPPINITFCS16, buf, sizeof(buf) );
    uint32_t crc32_num = crc32( PPPINITFCS32, buf, sizeof(buf) );
    CHECK_EQUAL( 0x7F, crc8_num );
    CHECK_EQUAL( GOODCHECKSUM, chksum( 0xFFFF - crc8_num, &crc8_num, 1 ) );
    CHECK_EQUAL( GOODCHECKSUM, 0xFFFF - chksum_byte( 0xFFFF - crc8_num, crc8_num ) );
    uint16_t crc16_swap = crc16_num;
    CHECK_EQUAL( 0x303C, crc16_num );
    CHECK_EQUAL( PPPGOODFCS16, (uint16_t)(crc16( crc16_num ^ ~0U, (uint8_t *)&crc16_swap, 2 ) ^ ~0U) );
    CHECK_EQUAL( PPPGOODFCS16, (uint16_t)(crc16_byte(
                                          crc16_byte( crc16_num ^ ~0U, crc16_num & 0xFF), crc16_num >> 8)) );
    uint32_t crc32_swap = crc32_num;
    CHECK_EQUAL( 0x29058C73, crc32_num );
    CHECK_EQUAL( PPPGOODFCS32, (uint32_t)(crc32( crc32_num ^ ~0U, (uint8_t *)&crc32_swap, 4 ) ^ ~0U) );
    CHECK_EQUAL( PPPGOODFCS32, (uint32_t)(crc32_byte( crc32_byte( crc32_byte( crc32_byte( crc32_num ^ ~0U,
                                          crc32_num & 0xFF ), (crc32_num >> 8) & 0xFF ),
                                          (crc32_num >> 16) & 0xFF), (crc32_num >> 24))) );
}
