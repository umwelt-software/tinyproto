/*
    Copyright 2017-2020 (C) Alexey Dynda

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
#include <TinyProtocolHdlc.h>

TEST_GROUP(HDLC){void setup(){
    // ...
}

                 void teardown(){
                     // ...
                 }};

TEST(HDLC, crc_mismatch)
{
    const char *txbuf = "This is CRC mismatch check";
    FakeConnection conn;
    TinyHdlcHelper helper1(&conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_32);
    TinyHdlcHelper helper2(&conn.endpoint2(), nullptr, nullptr, 1024, HDLC_CRC_16);
    int msg_size = strlen(txbuf) + 1;
    helper1.send((uint8_t *)txbuf, msg_size, 100);
    int result;
    do
    {
        result = helper2.run(false);
    } while ( result >= 0 );
    CHECK_EQUAL(TINY_ERR_WRONG_CRC, result);
}

TEST(HDLC, zero_len_not_allowed)
{
    FakeConnection conn;
    TinyHdlcHelper helper1(&conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_32);
    uint8_t buff[2];
    CHECK_EQUAL(TINY_ERR_INVALID_DATA, helper1.send(buff, 0, 100));
}

TEST(HDLC, crc8)
{
    const char *txbuf = "This is CRC8 check";
    FakeConnection conn;
    TinyHdlcHelper helper1(&conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_8);
    TinyHdlcHelper helper2(
        &conn.endpoint2(), [&txbuf](uint8_t *buf, int len) -> void { STRCMP_EQUAL(txbuf, (char *)buf); }, nullptr, 1024,
        HDLC_CRC_8);
    int msg_size = strlen(txbuf) + 1;
    helper1.send((uint8_t *)txbuf, msg_size, 100);
    helper2.wait_until_rx_count(1, 100);
    CHECK_EQUAL(1, helper2.rx_count());
}

TEST(HDLC, crc16)
{
    const char *txbuf = "This is CRC16 check";
    FakeConnection conn;
    TinyHdlcHelper helper1(&conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_16);
    TinyHdlcHelper helper2(
        &conn.endpoint2(), [&txbuf](uint8_t *buf, int len) -> void { STRCMP_EQUAL(txbuf, (char *)buf); }, nullptr, 1024,
        HDLC_CRC_16);
    int msg_size = strlen(txbuf) + 1;
    helper1.send((uint8_t *)txbuf, msg_size, 100);
    helper2.wait_until_rx_count(1, 100);
    CHECK_EQUAL(1, helper2.rx_count());
}

TEST(HDLC, crc32)
{
    const char *txbuf = "This is CRC32 check";
    FakeConnection conn;
    TinyHdlcHelper helper1(&conn.endpoint1(), nullptr, nullptr, 1024, HDLC_CRC_32);
    TinyHdlcHelper helper2(
        &conn.endpoint2(), [&txbuf](uint8_t *buf, int len) -> void { STRCMP_EQUAL(txbuf, (char *)buf); }, nullptr, 1024,
        HDLC_CRC_32);
    int msg_size = strlen(txbuf) + 1;
    helper1.send((uint8_t *)txbuf, msg_size, 100);
    helper2.wait_until_rx_count(1, 100);
    CHECK_EQUAL(1, helper2.rx_count());
}

TEST(HDLC, send_receive)
{
    uint32_t bytes_sent = 0;
    uint32_t bytes_received = 0;
    FakeConnection conn;
    TinyHdlcHelper helper1(&conn.endpoint1(), nullptr,
                           [&bytes_sent](uint8_t *buf, int len) -> void { bytes_sent += len; });
    TinyHdlcHelper helper2(&conn.endpoint2(),
                           [&bytes_received](uint8_t *buf, int len) -> void { bytes_received += len; });

    uint8_t txbuf[128];
    for ( int i = 0; i < 32; i++ )
    {
        snprintf((char *)txbuf, sizeof(txbuf) - 1, "This is frame Number %u (stream %i)", i, 0);
        int msg_size = strlen((char *)txbuf) + 1;
        int result = helper1.send((uint8_t *)txbuf, msg_size, 100);
        CHECK_EQUAL(TINY_SUCCESS, result);
        helper2.wait_until_rx_count(i + 1, 100);
        CHECK_EQUAL(i + 1, helper2.rx_count());
    }
    CHECK_EQUAL(0, conn.lostBytes());
    CHECK_EQUAL(helper1.tx_count(), helper2.rx_count());
    CHECK_EQUAL(bytes_sent, bytes_received);
}

TEST(HDLC, arduino_to_pc)
{
    uint8_t ardu_buffer[512];
    int timed_out_frames = 0;
    // PC side has a larger buffer
    FakeConnection conn(4096, 64);
    TinyHdlcHelper pc(&conn.endpoint1(), nullptr, nullptr, 512);
    TinyHdlcHelper arduino(
        &conn.endpoint2(),
        [&ardu_buffer, &arduino, &timed_out_frames](uint8_t *buf, int len) -> void {
            memcpy(ardu_buffer, buf, len);
            if ( arduino.send(ardu_buffer, len, 0) == TINY_ERR_BUSY )
            {
                timed_out_frames++;
            }
        },
        nullptr, 512);
    conn.setSpeed(115200);
    conn.endpoint2().setTimeout(0);
    conn.endpoint2().disable();
    arduino.setMcuMode(); // for hdlc it is important
    pc.run(true);
    // sent 100 small frames in background
    pc.send(100, "Generated frame. test in progress");
    // Usually arduino starts later by 2 seconds due to reboot on UART-2-USB access, emulate at teast 100ms delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    conn.endpoint2().enable();
    uint32_t startTs = tiny_millis();
    int tx_count = pc.tx_count() + 1;
    do
    {
        arduino.run_rx();
        arduino.run_tx();
        if ( static_cast<uint32_t>(tiny_millis() - startTs) > 2000 )
        {
            FAIL("Timeout happened");
            break;
        }
    } while ( pc.tx_count() != 100 || arduino.rx_count() < ((conn.lostBytes() ? 100 : 95) - tx_count) ||
              arduino.rx_count() > timed_out_frames + arduino.tx_count() ||
              pc.rx_count() < arduino.tx_count() - (conn.lostBytes() ? 5 : 0) );

    CHECK_EQUAL(100, pc.tx_count());
    if ( arduino.rx_count() < (conn.lostBytes() ? 100 : 95) - tx_count )
        CHECK_EQUAL(100 - tx_count, arduino.rx_count());
    // Some frames can be lost on send due to very small tx buffer on arduino side
    if ( arduino.rx_count() - timed_out_frames > arduino.tx_count() )
    {
        fprintf(stderr, "Lost bytes %d\n", conn.lostBytes());
        CHECK_EQUAL(arduino.rx_count() - timed_out_frames, arduino.tx_count());
    }
    if ( arduino.tx_count() > pc.rx_count() + (conn.lostBytes() ? 5 : 0) )
        CHECK_EQUAL(arduino.tx_count(), pc.rx_count());
    if ( conn.lostBytes() > 30 )
        CHECK_EQUAL(0, conn.lostBytes());
}

TEST(HDLC, single_receive)
{
    FakeConnection conn;
    conn.endpoint1().setTimeout(0);
    conn.endpoint2().setTimeout(100);
    TinyHdlcHelper helper2(&conn.endpoint2(), nullptr, nullptr, 1024, HDLC_CRC_OFF);
    uint32_t start_ts = tiny_millis();
    const uint8_t frame[] = {0x7E, 0x01, 0x02, 0x03, 0x7E};
    conn.endpoint1().write(frame, sizeof(frame));
    helper2.run(true);
    helper2.wait_until_rx_count(1, 50);
    CHECK_EQUAL(1, helper2.rx_count());
    if ( static_cast<uint32_t>(tiny_millis() - start_ts) > 50 )
    {
        FAIL("Timeout");
    }
}

TEST(HDLC, single_send)
{
    FakeConnection conn;
    conn.endpoint1().setTimeout(0);
    conn.endpoint2().setTimeout(10);
    TinyHdlcHelper helper2(&conn.endpoint2(), nullptr, nullptr, 1024, HDLC_CRC_OFF);
    const uint8_t frame[] = {0x7E, 0x01, 0x02, 0x03, 0x7E};
    helper2.send(frame + 1, sizeof(frame) - 2, 100);
    if ( !conn.endpoint1().wait_until_rx_count(sizeof(frame), 50) )
    {
        FAIL("Timeout");
    }
    uint8_t data[sizeof(frame)]{};
    conn.endpoint1().read(data, sizeof(data));
    MEMCMP_EQUAL(frame, data, sizeof(frame));
}

TEST(HDLC, hdlc_ll_missalignment)
{
    // Test to send 7E 7E XX XX 7E

    // On frame sent is not defined
    FakeConnection conn;
    conn.endpoint1().setTimeout(0);
    conn.endpoint2().setTimeout(10);
    TinyHdlcHelper helper2(&conn.endpoint2(), nullptr, nullptr, 1024, HDLC_CRC_OFF);
    const uint8_t data[] = { 0x7E, 0x7E, 0x01, 0x02, 0x03, 0x7E };
    conn.endpoint1().write(data, sizeof(data));

    helper2.wait_until_rx_count(2, 50); // Never 2 messages should be received, alway exit on timeout 50ms
    CHECK_EQUAL( 1, helper2.rx_count() );
}

TEST(HDLC, hdlc_send_escape_chars_encode)
{
    FakeConnection conn;
    conn.endpoint1().setTimeout(0);
    conn.endpoint2().setTimeout(10);
    TinyHdlcHelper helper2( &conn.endpoint2(), nullptr, nullptr, 1024, HDLC_CRC_OFF );
    const uint8_t frame[] = { 0x7E, 0x7D };
    helper2.send(frame, sizeof(frame), 100);

    const uint8_t data[] = { 0x7E, 0x7D, 0x5E, 0x7D, 0x5D, 0x7E };
    if ( !conn.endpoint1().wait_until_rx_count(sizeof(data), 50) )
    {
        FAIL("Timeout");
    }
    uint8_t actual_data[sizeof(data)]{};
    conn.endpoint1().read(actual_data, sizeof(actual_data));
    MEMCMP_EQUAL(data, actual_data, sizeof(data));
}

TEST(HDLC, hdlc_recv_escape_chars_decode)
{
    FakeConnection conn;
    conn.endpoint1().setTimeout(0);
    conn.endpoint2().setTimeout(10);
    TinyHdlcHelper helper2(&conn.endpoint2(), nullptr, nullptr, 1024, HDLC_CRC_OFF);
    const uint8_t data[] = { 0x7E, 0x7D, 0x5E, 0x7D, 0x5D, 0x7E };
    conn.endpoint1().write(data, sizeof(data));
    // Use single thread recv() implementation, that's why helper2.run( true ) is commented out
    // helper2.run(true);
    uint8_t frame[] = { 0x7E, 0x7D };
    uint8_t actual_frame[] = { 0x00, 0x00, 0x00 };
    int len = helper2.recv( actual_frame, sizeof(actual_frame), 50 );
    CHECK_EQUAL(sizeof(frame), len );
    MEMCMP_EQUAL( frame, actual_frame, len);
}
