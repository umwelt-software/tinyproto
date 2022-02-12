/*
    Copyright 2022 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    GNU General Public License Usage

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

    Commercial License Usage

    Licensees holding valid commercial Tiny Protocol licenses may use this file in
    accordance with the commercial license agreement provided in accordance with
    the terms contained in a written agreement between you and Alexey Dynda.
    For further information contact via email on github account.
*/

#include <functional>
#include <CppUTest/TestHarness.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include "helpers/tiny_fd_helper.h"
#include "helpers/fake_connection.h"

TEST_GROUP(FD_MULTI)
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

TEST(FD_MULTI, send_to_secondary)
{
    FakeSetup conn;
    FakeEndpoint &endpoint1 = conn.endpoint1();
    FakeEndpoint &endpoint2 = conn.endpoint2();
    TinyHelperFd primary(&endpoint1, 4096, TINY_FD_MODE_NRM, nullptr);
    TinyHelperFd secondary(&endpoint2, 4096, TINY_FD_MODE_NRM, nullptr);

    primary.setAddress( TINY_FD_PRIMARY_ADDR );
    primary.setTimeout( 250 );
    primary.init();

    secondary.setAddress( 1 );
    secondary.setTimeout( 250 );
    secondary.init();

    // Run secondary station first, as it doesn't transmit until primary sends a marker
    secondary.run(true);
    primary.run(true);

    CHECK_EQUAL(TINY_SUCCESS, primary.registerPeer( 1 ) );

    // send 1 small packets
    uint8_t txbuf[4] = {0xAA, 0xFF, 0xCC, 0x66};
    int result = primary.sendto(1, txbuf, sizeof(txbuf));
    CHECK_EQUAL(TINY_SUCCESS, result);

    result = primary.sendto(2, txbuf, sizeof(txbuf));
    CHECK_EQUAL(TINY_ERR_UNKNOWN_PEER, result);

    // wait until last frame arrives
    secondary.wait_until_rx_count(1, 250);
    CHECK_EQUAL(1, secondary.rx_count());
}

TEST(FD_MULTI, send_to_secondary_dual)
{
    FakeSetup conn;
    FakeEndpoint &endpoint1 = conn.endpoint1();
    FakeEndpoint &endpoint2 = conn.endpoint2();
    FakeEndpoint  endpoint3(conn.line2(), conn.line1(), 256, 256);
    TinyHelperFd primary(&endpoint1, 4096, TINY_FD_MODE_NRM, nullptr);
    TinyHelperFd secondary(&endpoint2, 4096, TINY_FD_MODE_NRM, nullptr);
    TinyHelperFd secondary2(&endpoint3, 4096, TINY_FD_MODE_NRM, nullptr);

    primary.setAddress( TINY_FD_PRIMARY_ADDR );
    primary.setTimeout( 250 );
    primary.setPeersCount( 2 );
    primary.init();

    secondary.setAddress( 1 );
    secondary.setTimeout( 250 );
    secondary.init();

    secondary2.setAddress( 2 );
    secondary2.setTimeout( 250 );
    secondary2.init();

    // Run secondary station first, as it doesn't transmit until primary sends a marker
    secondary.run(true);
    secondary2.run(true);
    primary.run(true);

    CHECK_EQUAL(TINY_SUCCESS, primary.registerPeer( 1 ) );
    CHECK_EQUAL(TINY_SUCCESS, primary.registerPeer( 2 ) );

    // send 1 small packets
    uint8_t txbuf[4] = {0xAA, 0xFF, 0xCC, 0x66};
    int result = primary.sendto(1, txbuf, sizeof(txbuf));
    CHECK_EQUAL(TINY_SUCCESS, result);

    // wait until last frame arrives
    secondary.wait_until_rx_count(1, 250);
    CHECK_EQUAL(1, secondary.rx_count());
    CHECK_EQUAL(0, secondary2.rx_count());

    result = primary.sendto(2, txbuf, sizeof(txbuf));
    secondary2.wait_until_rx_count(1, 250);
    CHECK_EQUAL(TINY_SUCCESS, result);
    CHECK_EQUAL(1, secondary.rx_count());
    CHECK_EQUAL(1, secondary2.rx_count());
}
