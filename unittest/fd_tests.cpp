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
#include <thread>
#include "helpers/tiny_fd_helper.h"
#include "helpers/fake_connection.h"


TEST_GROUP(FD)
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
TEST(FD, multithread_basic_test)
{
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr );
    helper1.run(true);
    helper2.run(true);

    // sent 200 small packets
    for (nsent = 0; nsent < 200; nsent++)
    {
        uint8_t      txbuf[4] = { 0xAA, 0xFF, 0xCC, 0x66 };
        int result = helper2.send( txbuf, sizeof(txbuf) );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // wait until last frame arrives
    helper1.wait_until_rx_count( 200, 1000 );
    CHECK_EQUAL( 200, helper1.rx_count() );
}
#endif

#if 1
TEST(FD, arduino_to_pc)
{
    std::atomic<int> low_device_frames{};
    FakeConnection conn( 4096, 32 ); // PC side has larger UART buffer: 4096
    TinyHelperFd pc( &conn.endpoint1(), 4096, nullptr );
    TinyHelperFd arduino( &conn.endpoint2(), tiny_fd_buffer_size_by_mtu(64,4),
                          [&arduino, &low_device_frames](uint16_t,uint8_t*b,int s)->void
                          { if ( arduino.send(b, s) == TINY_ERR_TIMEOUT ) low_device_frames++; }, 4, 0 );
    conn.endpoint2().setTimeout( 0 );
    conn.endpoint2().disable();
    conn.setSpeed( 115200 );
    pc.run(true);
    pc.send( 100, "Generated frame. test in progress" );
    // Usually arduino starts later by 2 seconds due to reboot on UART-2-USB access, emulate al teast 100ms delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    conn.endpoint2().enable();
    // sent 200 small packets
    auto startTs = std::chrono::steady_clock::now();
    do
    {
        arduino.run_rx();
        arduino.run_tx();
    } while ( pc.rx_count() + low_device_frames != 100 && std::chrono::steady_clock::now() - startTs <= std::chrono::seconds(4) );
    // Not lost bytes check for now, we admit, that FD hdlc can restransmit frames on bad lines
//    CHECK_EQUAL( 0, conn.lostBytes() );
    CHECK_EQUAL( 100 - low_device_frames, pc.rx_count() );
}
#endif

#if 1
TEST(FD, errors_on_tx_line)
{
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr );
    conn.line2().generate_error_every_n_byte( 200 );
    helper1.run(true);
    helper2.run(true);

    for (nsent = 0; nsent < 200; nsent++)
    {
        uint8_t      txbuf[4] = { 0xAA, 0xFF, 0xCC, 0x66 };
        int result = helper2.send( txbuf, sizeof(txbuf) );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // wait until last frame arrives
    helper1.wait_until_rx_count( 200, 1000 );
    CHECK_EQUAL( 200, helper1.rx_count() );
}
#endif

#if 1
TEST(FD, error_on_single_I_send)
{
    // Each U-frame or S-frame is 6 bytes or more: 7F, ADDR, CTL, FSC16, 7F
    // TX1: U, U, R
    // TX2: U, U, I,
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, 7, 2000 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, 7, 2000 );
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
TEST(FD, error_on_rej)
{
    // Each U-frame or S-frame is 6 bytes or more: 7F, ADDR, CTL, FSC16, 7F
    // TX1: U, U, R
    // TX2: U, U, I,
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, 7, 2000 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, 7, 2000 );
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
TEST(FD, singlethread_basic)
{
    // TODO:
    CHECK_EQUAL( 0, 0 );
}
#endif
