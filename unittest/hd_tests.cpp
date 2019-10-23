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


#include <functional>
#include <CppUTest/TestHarness.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helpers/tiny_hd_helper.h"
#include "helpers/fake_connection.h"


TEST_GROUP(HD)
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
TEST(HD, singlethread)
{
    const int bufsize = 16;
    const int msgnum = 2;

    FakeConnection conn;
    conn.endpoint1().setTimeout( 100 );
    conn.endpoint2().setTimeout( 100 );
    // Do not use multithread mode
    TinyHelperHd helper1( &conn.endpoint1(), bufsize, nullptr, true);
    TinyHelperHd helper2( &conn.endpoint2(), bufsize, nullptr, false );

    helper1.run(true);

    // sent 4 packets small packets with ACK
    for (int nsent = 0; nsent < msgnum; nsent++)
    {
        uint8_t txbuf[bufsize];
        txbuf[0] = 0xAA;
        txbuf[1] = 0xFF;
        int result = helper2.send_wait_ack( txbuf, 2 );
        CHECK_EQUAL( 2, result );
    }
//    helper1.wait_until_rx_count( msgnum, 1000 );
    CHECK_EQUAL( msgnum, helper1.rx_count() );
}
#endif

#if 1
TEST(HD, multithread)
{
    const int bufsize = 16;
    const int msgnum = 2;
    FakeConnection conn;
    conn.endpoint1().setTimeout( 100 );
    conn.endpoint2().setTimeout( 100 );

    TinyHelperHd helper1( &conn.endpoint1(), bufsize, nullptr, true);
    TinyHelperHd helper2( &conn.endpoint2(), bufsize, nullptr, true );

    helper1.run(true);
    helper2.run(true);
    // sent 4 packets small packets with ACK
    for (int nsent = 0; nsent < msgnum; nsent++)
    {
        uint8_t      txbuf[bufsize];
        txbuf[0] = 0xAA;
        txbuf[1] = 0xFF;
        int result = helper2.send_wait_ack( txbuf, 2 );
        CHECK_EQUAL( 2, result );
    }
//    helper1.wait_until_rx_count( msgnum, 1000 );
    CHECK_EQUAL( 0, conn.lostBytes() );
    CHECK_EQUAL( msgnum, helper1.rx_count() );
}
#endif
