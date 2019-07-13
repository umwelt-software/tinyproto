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
    FakeWire     line1;
    FakeWire     line2;
    FakeChannel  channel1( &line1, &line2 );
    FakeChannel  channel2( &line2, &line1 );
    TinyHdlcHelper   helper1( &channel1 );
    TinyHelper   helper2( &channel2 );

    uint8_t      txbuf[128];
    uint8_t      rxbuf[128];
    uint16_t     nsent = 0;
    uint16_t     nreceived = 0;
    uint8_t      flags = TINY_FLAG_NO_WAIT | TINY_FLAG_LOCK_SEND;

    while (nsent < 32)
    {
        snprintf((char *)txbuf, sizeof(txbuf) - 1, "This is frame Number %u (stream %i)", nsent, 0);
        int result = helper1.send( NULL, (uint8_t *)txbuf, strlen((char *)txbuf) + 1, flags);
        printf("LEN: %d, NLEN: %d \n", result, (int)strlen((char *)txbuf) + 1);
        CHECK( (result == TINY_NO_ERROR) || (result == ((int)strlen((char *)txbuf) + 1 + 4)) );
        if ( result == TINY_NO_ERROR )
        {
            flags = TINY_FLAG_NO_WAIT;
        }
        else if ( result == (int)strlen((char *)txbuf) + 1 + 4 )
        {
            flags = TINY_FLAG_NO_WAIT | TINY_FLAG_LOCK_SEND;
            nsent++;
        }
        result = helper2.read( NULL, (uint8_t *)rxbuf, sizeof(rxbuf), TINY_FLAG_NO_WAIT);
        printf( "result = %d\n", result );
        CHECK( (result == TINY_NO_ERROR) || (result > 0) );
        if ( result > 0 )
        {
            // strlen + \0 + sizeof(uid)
            CHECK_EQUAL( (int)strlen((const char*)rxbuf) + 1, result );
        }
        if (result > 0)
        {
            nreceived++;
        }
    }
    CHECK_EQUAL( nreceived, nsent );
}

#endif

#if 0
TEST(HdlcTests, TinyLayer2_Send_Receive_Event_Based)
{
    uint16_t     nreceived = 0;
    FakeWire     line1;
    FakeWire     line2;
    FakeChannel  channel1( &line1, &line2 );
    FakeChannel  channel2( &line2, &line1 );
    TinyHdlcHelper   helper1( &channel1 );
    TinyHelper   helper2( &channel2, [&nreceived](uint16_t uid,uint8_t *buf, int len)->void
                          {
                              uint32_t sum_expected = 0;
                              uint32_t sum_got = 0;
                              for(int j=0; j<uid; j++) sum_expected += (j+uid) % 256;
                              for(int j=0; j<len; j++) sum_got += buf[j];
                              CHECK_EQUAL(sum_got, sum_expected);
                              nreceived++;
                          } );

    uint8_t      txbuf[128];
    uint8_t      rxbuf[128];
    uint16_t     nsent = 0;
    uint8_t      flags = TINY_FLAG_NO_WAIT | TINY_FLAG_LOCK_SEND;

//    helper1.enableUid();
//    helper2.enableUid();
    /* uint16_t is size of uid, which is part of received data */
    while (nsent < sizeof(txbuf) - sizeof(uint16_t))
    {
        for(uint16_t i=0; i<nsent; i++) txbuf[i] = ( i+nsent ) % 256;
        int result = helper1.send( &nsent, (uint8_t *)txbuf, nsent, flags);
        CHECK( (result == TINY_NO_ERROR) || (result == TINY_ERR_TIMEOUT) || (result == nsent) || (result == TINY_SUCCESS) );
        if ( result == TINY_NO_ERROR )
        {
            flags = TINY_FLAG_NO_WAIT;
        }
        else if ( result > 0 )
        {
            flags = TINY_FLAG_NO_WAIT | TINY_FLAG_LOCK_SEND;
            nsent++;
        }
        uint8_t byte;
        while ( channel2.read(&byte, 1) == 1 )
        {
            int res = helper2.on_rx_byte( rxbuf, sizeof(rxbuf), byte );
            if (res < 0) printf("failed:%i\n", res);
            CHECK( res >= 0 );
        }
    }
    CHECK_EQUAL( nreceived, nsent );
}

#endif
