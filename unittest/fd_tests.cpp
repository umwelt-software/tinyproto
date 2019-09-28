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
TEST(FdTests, TinyFd_Multithread)
{
    uint8_t      txbuf[4096];
    FakeWire     line1;
    FakeWire     line2;
    FakeChannel  channel1( &line1, &line2 );
    FakeChannel  channel2( &line2, &line1 );
    uint16_t     nreceived = 0;
    uint16_t     nsent = 0;

    // Do not use multithread mode
    TinyHelperFd helper1( &channel1,
                          sizeof(txbuf),
                          [&nreceived](uint16_t uid, uint8_t *buf, int len)->void
                          {
//                              printf("received uid:%04X, len: %d\n", uid, len);
                              nreceived++;
                          }, true);
    TinyHelperFd helper2( &channel2, sizeof(txbuf), nullptr, true );

//    line2.generate_error_on_byte( 200 );
    helper1.run(true);
    helper2.run(true);

    // sent 4 packets small packets with ACK
    for (nsent = 0; nsent < 500; nsent++)
    {
        txbuf[0] = 0xAA;
        txbuf[1] = 0xFF;
        txbuf[2] = 0xCC;
        txbuf[3] = 0x66;
        //printf("sending %d bytes\n", 4);
        int result = helper2.send( txbuf, 4, 1000 );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // sleep for 50 ms before last frame arrives
    usleep(50000);
    CHECK_EQUAL( 500, nreceived );
}
#endif

#if 0
TEST(HdTests, TinyHd_Singlethread)
{
    uint8_t      txbuf[4096];
    FakeWire     line1;
    FakeWire     line2;
    FakeChannel  channel1( &line1, &line2 );
    FakeChannel  channel2( &line2, &line1 );
    uint16_t     nreceived = 0;
    uint16_t     nsent = 0;

    // Do not use multithread mode
    TinyHelperHd helper1( &channel1,
                          sizeof(txbuf),
                          [&nreceived](uint16_t uid, uint8_t *buf, int len)->void
                          {
//                              printf("received uid:%04X, len: %d\n", uid, len);
                              nreceived++;
                          }, true);
    TinyHelperHd helper2( &channel2, sizeof(txbuf), nullptr, true );

    helper1.run(true);
    helper2.run(true);

    // sent 4 packets small packets with ACK
    for (nsent = 0; nsent < 500; nsent++)
    {
        txbuf[0] = 0xAA;
        txbuf[1] = 0xFF;
//        printf("sending %d bytes\n", 2);
        int result = helper2.send_wait_ack( txbuf, 2 );
        CHECK_EQUAL( 2, result );
    }
    // sleep for 10 ms befoe last frame arrives
    usleep(10000);
    CHECK_EQUAL( 500, nreceived );
}

#endif
