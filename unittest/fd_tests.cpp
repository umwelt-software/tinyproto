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


#include <functional>
#include <CppUTest/TestHarness.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helpers/tiny_fd_helper.h"
#include "helpers/fake_connection.h"


TEST_GROUP(FdTests)
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

#if 1
TEST(FdTests, TinyFd_multithread_basic_test)
{
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, true);
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, true );
    helper1.run(true);
    helper2.run(true);

    // sent 500 small packets
    for (nsent = 0; nsent < 500; nsent++)
    {
        uint8_t      txbuf[4] = { 0xAA, 0xFF, 0xCC, 0x66 };
        int result = helper2.send( txbuf, sizeof(txbuf) );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // wait until last frame arrives
    helper1.wait_until_rx_count( 500, 1000 );
    CHECK_EQUAL( 500, helper1.rx_count() );
}
#endif

#if 1
TEST(FdTests, TinyFd_errors_on_tx_line)
{
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, true);
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, true );
    conn.line2().generate_error_every_n_byte( 200 );
    helper1.run(true);
    helper2.run(true);

    // sent 500 small packets
    for (nsent = 0; nsent < 300; nsent++)
    {
        uint8_t      txbuf[4] = { 0xAA, 0xFF, 0xCC, 0x66 };
        int result = helper2.send( txbuf, sizeof(txbuf) );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // wait until last frame arrives
    helper1.wait_until_rx_count( 300, 1000 );
    CHECK_EQUAL( 300, helper1.rx_count() );
}
#endif

#if 1
TEST(FdTests, TinyFd_error_on_single_I_send)
{
    // Each U-frame or S-frame is 6 bytes or more: 7F, ADDR, CTL, FSC16, 7F
    // TX1: U, U, R
    // TX2: U, U, I,
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, true, 2000 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, true, 2000 );
    conn.line2().generate_single_error( 6 + 6 + 3 ); // Put error on I-frame
    helper1.run(true);
    helper2.run(true);

    for (nsent = 0; nsent < 1; nsent++)
    {
        uint8_t      txbuf[4] = { 0xAA, 0xFF, 0xCC, 0x66 };
        int result = helper2.send( txbuf, sizeof(txbuf) );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // wait until last frame arrives
    helper1.wait_until_rx_count( 1, 2000 );
    CHECK_EQUAL( 1, helper1.rx_count() );
}
#endif

#if 1
TEST(FdTests, TinyFd_error_on_rej)
{
    // Each U-frame or S-frame is 6 bytes or more: 7F, ADDR, CTL, FSC16, 7F
    // TX1: U, U, R
    // TX2: U, U, I,
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, true, 2000 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, true, 2000 );
    conn.line2().generate_single_error( 6 + 6 + 4 ); // Put error on first I-frame
    conn.line1().generate_single_error( 6 + 6 + 3 ); // Put error on S-frame REJ
    helper1.run(true);
    helper2.run(true);

    for (nsent = 0; nsent < 2; nsent++)
    {
        uint8_t      txbuf[4] = { 0xAA, 0xFF, 0xCC, 0x66 };
        int result = helper2.send( txbuf, sizeof(txbuf) );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // wait until last frame arrives
    helper1.wait_until_rx_count( 2, 2000 );
    CHECK_EQUAL( 2, helper1.rx_count() );
}
#endif

#if 1
TEST(FdTests, TinyHd_singlethread_basic)
{
    // TODO:
    CHECK_EQUAL( 0, 0 );
}
#endif
