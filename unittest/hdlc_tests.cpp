/*
    Copyright 2017-2018 (C) Alexey Dynda

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
#include "helpers/tiny_helper.h"
#include "helpers/tiny_hdlc_helper.h"


TEST_GROUP(HdlcTests)
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
TEST(HdlcTests, TinyLayer2_Send_Receive)
{
    uint16_t     nsent = 0;
    uint16_t     nreceived = 0;
    uint32_t     bytes_sent = 0;
    uint32_t     bytes_received = 0;
    FakeWire     line1;
    FakeWire     line2;
    FakeChannel  channel1( &line1, &line2 );
    FakeChannel  channel2( &line2, &line1 );
    TinyHdlcHelper   helper1( &channel1,
                            nullptr,
                            [&nsent,&bytes_sent](uint8_t *buf, int len)->void
                            {
                                bytes_sent += len;
                                nsent++;
                            } );
    TinyHdlcHelper   helper2( &channel2,
                            [&nreceived,&bytes_received](uint8_t *buf, int len)->void
                            {
                                bytes_received += len;
                                nreceived++;
                            }
                            );
    uint8_t      txbuf[128];

    while (nsent < 32)
    {
        snprintf((char *)txbuf, sizeof(txbuf) - 1, "This is frame Number %u (stream %i)", nsent, 0);
        int result = helper1.send( (uint8_t *)txbuf, strlen((char *)txbuf) + 1 );
        CHECK( result > 0 );
        result = helper2.process_rx_bytes();
        if (result < 0) printf("failed:%i\n", result);
        CHECK( result >= 0 );
    }
    CHECK_EQUAL( nsent, nreceived );
    CHECK_EQUAL( bytes_sent, bytes_received );
}

#endif

