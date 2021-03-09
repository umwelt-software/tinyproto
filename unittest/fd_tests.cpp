/*
    Copyright 2019-2020 (C) Alexey Dynda

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
//        fprintf(stderr, "======== START =======\n" );
    }

    void teardown()
    {
        // ...
//        fprintf(stderr, "======== END =======\n" );
    }
};

TEST(FD, multithread_basic_test)
{
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, 7, 250 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, 7, 250 );
    // Vaildation of tiny_fd_run_rx()
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
    helper1.wait_until_rx_count( 200, 250 );
    CHECK_EQUAL( 200, helper1.rx_count() );
}

TEST(FD, multithread_read_test)
{
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, 7, 250 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, 7, 250 );
    // Validation of tiny_fd_on_rx_data()
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
    helper1.wait_until_rx_count( 200, 250 );
    CHECK_EQUAL( 200, helper1.rx_count() );
}

TEST(FD, arduino_to_pc)
{
    std::atomic<int> arduino_timedout_frames{};
    FakeConnection conn( 4096, 64 ); // PC side has larger UART buffer: 4096, arduino side has small uart buffer
    TinyHelperFd pc( &conn.endpoint1(), 4096, nullptr, 4, 400 );
    TinyHelperFd arduino( &conn.endpoint2(), tiny_fd_buffer_size_by_mtu(64,4),
                          [&arduino, &arduino_timedout_frames](uint8_t*b,int s)->void
                          { if ( arduino.send(b, s) == TINY_ERR_TIMEOUT ) arduino_timedout_frames++; }, 4, 0 );
    conn.endpoint2().setTimeout( 0 );
    conn.endpoint2().disable();
    conn.setSpeed( 115200 );
    pc.run(true);
    // sent 100 small packets from pc to arduino
    pc.send( 100, "Generated frame. test in progress" );
    // Usually arduino starts later by 2 seconds due to reboot on UART-2-USB access, emulate al teast 100ms delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    conn.endpoint2().enable();
    uint32_t start_ms = tiny_millis();
    do
    {
        arduino.run_rx();
        arduino.run_tx();
        if ( static_cast<uint32_t>(tiny_millis() - start_ms) > 2000 ) break;
    } while ( pc.tx_count() != 100 &&  pc.rx_count() + arduino_timedout_frames < 99 );
    // it is allowed to miss several frames due to arduino cycle completes before last messages are delivered
    if ( 95 - arduino_timedout_frames >  pc.rx_count() )
    {
        CHECK_EQUAL( 100 - arduino_timedout_frames,  pc.rx_count() );
    }
}

TEST(FD, errors_on_tx_line)
{
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 1024, nullptr, 7, 250 );
    TinyHelperFd helper2( &conn.endpoint2(), 1024, nullptr, 7, 250 );
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
    helper1.wait_until_rx_count( 200, 250 );
    CHECK_EQUAL( 200, helper1.rx_count() );
}

TEST(FD, error_on_single_I_send)
{
    // Each U-frame or S-frame is 6 bytes or more: 7F, ADDR, CTL, FSC16, 7F
    // TX1: U, U, R
    // TX2: U, U, I,
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, 7, 1000 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, 7, 1000 );
    conn.line2().generate_single_error( 6 + 6 + 3 ); // Put error on I-frame
    helper1.run(true);
    helper2.run(true);

    // retransmissiomn must happen very quickly since FD detects out-of-order frames
    for (nsent = 0; nsent < 2; nsent++)
    {
        uint8_t      txbuf[4] = { 0xAA, 0xFF, 0xCC, 0x66 };
        int result = helper2.send( txbuf, sizeof(txbuf) );
        CHECK_EQUAL( TINY_SUCCESS, result );
    }
    // wait until last frame arrives
    helper1.wait_until_rx_count( 2, 300 );
    CHECK_EQUAL( 2, helper1.rx_count() );
}

TEST(FD, error_on_rej)
{
    // Each U-frame or S-frame is 6 bytes or more: 7F, ADDR, CTL, FSC16, 7F
    // TX1: U, U, R
    // TX2: U, U, I,
    FakeConnection conn;
    uint16_t     nsent = 0;
    TinyHelperFd helper1( &conn.endpoint1(), 4096, nullptr, 7, 250 );
    TinyHelperFd helper2( &conn.endpoint2(), 4096, nullptr, 7, 250 );
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
    helper1.wait_until_rx_count( 2, 250 );
    CHECK_EQUAL( 2, helper1.rx_count() );
}

TEST(FD, no_ka_switch_to_disconnected)
{
    FakeConnection conn(32, 32);
    TinyHelperFd helper1( &conn.endpoint1(), 1024, nullptr, 4, 100 );
    TinyHelperFd helper2( &conn.endpoint2(), 1024, nullptr, 4, 100 );
    conn.endpoint1().setTimeout( 30 );
    conn.endpoint2().setTimeout( 30 );
    helper1.set_ka_timeout( 100 );
    helper2.set_ka_timeout( 100 );
    helper1.run(true);
    helper2.run(true);

    // Consider FD use keep alive to keep connection during 50 milliseconds
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // Stop remote side, and sleep again for 150 milliseconds, to get disconnected state
    helper2.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    // Stop both endpoints to do clean flush
    helper1.stop();
    conn.endpoint1().flush();
    conn.endpoint2().flush();
    // Run local endpoint again.
    helper1.run(true);
    // At this step, we should be in disconnected state, and should see SABM frames
    // sleep for 100 milliseconds to get at least one Keep Alive
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint8_t buffer[32];
    int len = conn.endpoint2().read( buffer, sizeof(buffer) );
    const uint8_t sabm_request[] = { 0x7E, 0xFF, 0x3F, 0xF3, 0x39, 0x7E };
    if ( (size_t)len < sizeof(sabm_request) )
    {
         CHECK_EQUAL( sizeof(sabm_request), len );
    }
    MEMCMP_EQUAL( sabm_request, buffer, sizeof(sabm_request) );
}

TEST(FD, resend_timeout)
{
    FakeConnection conn(128, 128);
    TinyHelperFd helper1( &conn.endpoint1(), 1024, nullptr, 4, 70 );
    TinyHelperFd helper2( &conn.endpoint2(), 1024, nullptr, 4, 70 );
    conn.endpoint1().setTimeout( 30 );
    conn.endpoint2().setTimeout( 30 );
    helper1.run(true);
    helper2.run(true);

    // During 100ms FD must establish connection to remote side
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Stop remote side, and try to send something
    helper2.stop();
    conn.endpoint1().flush();
    conn.endpoint2().flush();
    helper1.send("#");
    std::this_thread::sleep_for(std::chrono::milliseconds(70*2 + 100));
    helper1.stop();
    const uint8_t reconnect_dat[] = { 0x7E, 0xFF, 0x00, '#', 0xA6, 0x13, 0x7E, // 1-st attempt
                                      0x7E, 0xFF, 0x00, '#', 0xA6, 0x13, 0x7E, // 2-nd attempt (1st retry)
                                      0x7E, 0xFF, 0x00, '#', 0xA6, 0x13, 0x7E, // 3-rd attempt (2nd retry)
                                      0x7E, 0xFF, 0x3F, 0xF3, 0x39, 0x7E }; // Attempt to reconnect (SABM)
    uint8_t buffer[64]{};
    conn.endpoint2().read( buffer, sizeof(buffer) );
    MEMCMP_EQUAL( reconnect_dat, buffer, sizeof(reconnect_dat) );
}

TEST(FD, singlethread_basic)
{
    // TODO:
    CHECK_EQUAL( 0, 0 );
}
