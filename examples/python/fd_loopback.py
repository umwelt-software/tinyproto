#
# Copyright 2021 (C) Alexey Dynda
#
# This file is part of Tiny Protocol Library.
#
# Protocol Library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Protocol Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
#

import argparse
import sys
import threading
import time

import serial

import tinyproto

g_sent_bytes: int = 0
g_received_bytes: int = 0
g_proto: tinyproto.Fd = None
g_stop: bool = False


def tx_thread_fn(endpoint: tinyproto.Fd, port: serial.Serial):
    def write_func(data):
        try:
            written = port.write(data)
        except Exception as e:
            print(f"Unable to write: {e}", file=sys.stderr)
            written = 0

        return written

    while not g_stop:
        endpoint.run_tx(write_func)


def rx_thread_fn(endpoint, port: serial.Serial):
    def read_func(max_count):
        try:
            data = port.read(max_count)
        except Exception as e:
            data = bytes()
            print(f"Unable to read: {e}", file=sys.stderr)
        data = bytearray(data)
        return data

    while not g_stop:
        endpoint.run_rx(read_func)


def on_read(data):
    if not g_args.run_test:
        print(f"<<< Frame received payload len={len(data)}", file=sys.stderr)

    global g_received_bytes
    g_received_bytes += len(data)
    if not g_args.generator:
        ret = g_proto.send(data)
        if ret < 0:
            print(f"Failed to send loopback packet {ret}", file=sys.stderr)


def on_send(data):
    if not g_args.run_test:
        print(f">>> Frame sent payload len={len(data)}", file=sys.stderr)

    global g_sent_bytes
    g_sent_bytes += len(data)


def main(port: str, crc: int, generator: bool, packet_size: int, run_test: bool):
    serial_port = serial.serial_for_url(port, baudrate=115200, timeout=0.1)
    global g_proto
    g_proto = tinyproto.Fd()
    g_proto.mtu = packet_size
    g_proto.crc = crc
    g_proto.on_read = on_read
    g_proto.on_send = on_send
    g_proto.begin()
    tx_thread = threading.Thread(
        target=tx_thread_fn, name="tx_thread", args=(g_proto, serial_port))
    tx_thread.start()
    rx_thread = threading.Thread(
        target=rx_thread_fn, name="rx_thread", args=(g_proto, serial_port))
    rx_thread.start()

    start_time = time.time()
    progress_time = start_time

    TEST_DURATION_SECONDS = 15

    global g_stop
    while not g_stop:
        if generator:
            ret = g_proto.send(b"Generated frame. test in progress")
            if ret < 0:
                print(f"Failed to send packet: {ret}", file=sys.stderr)
        else:
            time.sleep(1)

        if run_test and generator:
            now = time.time()
            if now - start_time >= TEST_DURATION_SECONDS:
                g_stop = True
            if now - progress_time >= 1:
                progress_time = now
                print(".", end="", file=sys.stderr, flush=True)

    tx_thread.join()
    rx_thread.join()
    g_proto.end()

    if run_test:
        print(f"\nRegistered TX speed: {int((g_sent_bytes) * 8 / TEST_DURATION_SECONDS)} bps")
        print(f"Registered RX speed: {int((g_received_bytes) * 8 / TEST_DURATION_SECONDS)} bps")


parser = argparse.ArgumentParser(
    description='Test throughput of the Python bindings of tinyproto')
parser.add_argument('-p', '--port', dest="port", type=str, required=True,
                    help='com port to use\nCOM1, COM2 ... for Windows\n/dev/ttyS0, /dev/ttyS1 ... for Linux')
parser.add_argument('-c', '--crc', dest="crc", type=int,
                    default=8, choices=[0, 8, 16, 32],
                    help='crc type')
parser.add_argument('-s', '--size', dest="packet_size", default=64, type=int,
                    help='packet size: 64 (by default)')
parser.add_argument('-g', '--generator', dest="generator", action='store_true', default=False,
                    help='turn on packet generating')
parser.add_argument('-r', '--run-test', dest="run_test", action="store_true", default=False,
                    help='run 15 seconds speed test')

if __name__ == '__main__':
    g_args = parser.parse_args()
    main(g_args.port, g_args.crc, g_args.generator,
         g_args.packet_size, g_args.run_test)
