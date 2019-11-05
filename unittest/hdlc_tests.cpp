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
#include "helpers/tiny_hdlc_helper.h"
#include "helpers/fake_connection.h"


TEST_GROUP(HDLC)
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

TEST(HDLC, crc_mismatch)
{
    const char *txbuf = "This is CRC mismatch check";
    FakeConnection conn;
    TinyHdlcHelper   helper1( &conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_32 );
    TinyHdlcHelper   helper2( &conn.endpoint2(), nullptr, nullptr, 1024, HDLC_CRC_16 );
    int msg_size = strlen(txbuf) + 1;
    helper1.send( (uint8_t *)txbuf, msg_size, 1000 );
    int result = helper2.run_rx_until_read();
    CHECK_EQUAL( TINY_ERR_WRONG_CRC, result );
}

TEST(HDLC, zero_len_not_allowed)
{
    FakeConnection conn;
    TinyHdlcHelper   helper1( &conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_32 );
    uint8_t buff[2];
    CHECK_EQUAL( TINY_ERR_INVALID_DATA, helper1.send( buff, 0, 100 ) );
}

TEST(HDLC, crc8)
{
    const char *txbuf = "This is CRC8 check";
    FakeConnection conn;
    TinyHdlcHelper   helper1( &conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_8 );
    TinyHdlcHelper   helper2( &conn.endpoint2(), [&txbuf](uint8_t *buf, int len)->void
                              { STRCMP_EQUAL( txbuf, (char *)buf ); },
                              nullptr, 1024, HDLC_CRC_8 );
    int msg_size = strlen(txbuf) + 1;
    helper1.send( (uint8_t *)txbuf, msg_size, 1000 );
    int result = helper2.run_rx_until_read();
    CHECK_EQUAL( msg_size, result );
}

TEST(HDLC, crc16)
{
    const char *txbuf = "This is CRC16 check";
    FakeConnection conn;
    TinyHdlcHelper   helper1( &conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_16 );
    TinyHdlcHelper   helper2( &conn.endpoint2(), [&txbuf](uint8_t *buf, int len)->void
                              { STRCMP_EQUAL( txbuf, (char *)buf ); },
                              nullptr, 1024, HDLC_CRC_16 );
    int msg_size = strlen(txbuf) + 1;
    helper1.send( (uint8_t *)txbuf, msg_size, 1000 );
    int result = helper2.run_rx_until_read();
    CHECK_EQUAL( msg_size, result );
}

TEST(HDLC, crc32)
{
    const char *txbuf = "This is CRC32 check";
    FakeConnection conn;
    TinyHdlcHelper   helper1( &conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_32 );
    TinyHdlcHelper   helper2( &conn.endpoint2(), [&txbuf](uint8_t *buf, int len)->void
                              { STRCMP_EQUAL( txbuf, (char *)buf ); },
                              nullptr, 1024, HDLC_CRC_32 );
    int msg_size = strlen(txbuf) + 1;
    helper1.send( (uint8_t *)txbuf, msg_size, 1000 );
    int result = helper2.run_rx_until_read();
    CHECK_EQUAL( msg_size, result );
}

TEST(HDLC, send_receive)
{
    uint32_t     bytes_sent = 0;
    uint32_t     bytes_received = 0;
    FakeConnection conn;
    TinyHdlcHelper   helper1( &conn.endpoint1(),
                            nullptr,
                            [&bytes_sent](uint8_t *buf, int len)->void
                            {
                                bytes_sent += len;
                            } );
    TinyHdlcHelper   helper2( &conn.endpoint2(),
                            [&bytes_received](uint8_t *buf, int len)->void
                            {
                                bytes_received += len;
                            } );

    uint8_t      txbuf[128];
    for (int i=0; i<32; i++)
    {
        snprintf((char *)txbuf, sizeof(txbuf) - 1, "This is frame Number %u (stream %i)", i, 0);
        int msg_size = strlen((char *)txbuf) + 1;
        int result = helper1.send( (uint8_t *)txbuf, msg_size, 1000 );
        CHECK_EQUAL( TINY_SUCCESS, result );
        result = helper2.run_rx_until_read();
        CHECK_EQUAL( msg_size, result );
    }
    CHECK_EQUAL( 0, conn.lostBytes() );
    CHECK_EQUAL( helper1.tx_count(), helper2.rx_count() );
    CHECK_EQUAL( bytes_sent, bytes_received );
}

TEST(HDLC, arduino_to_pc)
{
    uint8_t ardu_buffer[512];
    // PC side has a larger buffer
    FakeConnection conn( 128, 32 );
    TinyHdlcHelper pc( &conn.endpoint1(), nullptr, nullptr, 512 );
    TinyHdlcHelper arduino( &conn.endpoint2(), [&ardu_buffer, &arduino](uint8_t *buf, int len)->void {
                                memcpy( ardu_buffer, buf,len );
                                arduino.send( ardu_buffer, len, 0 );
                            }, nullptr, 512 );
    // TODO: Temporary go down from 115200 to 14400 due to lost bytes
    conn.setSpeed( 14400 );
    conn.endpoint2().setTimeout( 0 );
    conn.endpoint2().disable();
    arduino.setMcuMode(); // for hdlc it is important
    pc.run(true);
    // sent 100 small frames
    pc.send( 100, "Generated frame. test in progress" );
    // Usually arduino starts later by 2 seconds due to reboot on UART-2-USB access, emulate at teast 100ms delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    conn.endpoint2().enable();
    const int tx_count = pc.tx_count() + 2; // And 2 partially transferred frame
    auto startTs = std::chrono::steady_clock::now();
    do
    {
        arduino.run_rx();
        arduino.run_tx();
    } while ( (pc.tx_count() != 100 || pc.rx_count() < arduino.tx_count() -2) &&
              std::chrono::steady_clock::now() - startTs <= std::chrono::seconds(5) );
    CHECK_EQUAL( 100, pc.tx_count() );
    if ( pc.tx_count() - tx_count > arduino.rx_count() )
        CHECK_EQUAL( pc.tx_count() - tx_count, arduino.rx_count() );
    CHECK_EQUAL( arduino.rx_count(), arduino.tx_count() );
    if ( arduino.tx_count() - 2 > pc.rx_count() )
        CHECK_EQUAL( arduino.tx_count(), pc.rx_count() );
    CHECK_EQUAL( 0, conn.lostBytes() );
}

