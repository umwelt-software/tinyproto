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

import tinyproto
import threading, queue
import time

# Define global flag to indicate when threads are required to terminate
stop = False

"""
 This is simple endpoint processing function.
 It reads byte for rx queue and pass them to protocol, and it writes bytes from protocol to tx queue.
"""
def run_endpoint(endpoint, rx_queue, tx_queue):
    endpoint.begin()

    def read_func( max_size ):
        bytes = []
        while not rx_queue.empty() and max_size > 0:
            bytes.append( rx_queue.get( block = True ) )
            max_size -= 1
        return bytearray( bytes )

    def write_func( data ):
        for byte in data:
            tx_queue.put( byte, block = True, timeout = 0.1 )
        return len( data )

    while not stop:
        endpoint.run_tx( write_func );
        endpoint.run_rx( read_func )

    endpoint.end()


# Create protocol objects for local and remote end
local_side = tinyproto.Fd()
remote_side = tinyproto.Fd()

# setup local side protocol to report all messages being sent and received
def on_read(a):
    print("Received bytes: " + ','.join( [ "{:#x}".format(x) for x in a ] ) )

def on_send(a):
    print("Sent bytes: " + ','.join( [ "{:#x}".format(x) for x in a ] ) )

local_side.on_read = on_read
local_side.on_send = on_send

# Use queue to emulate UART hardware

tx_queue = queue.Queue(32)
rx_queue = queue.Queue(32)

# Start local side with RX / TX lines
local = threading.Thread(target=run_endpoint, args=(local_side, rx_queue, tx_queue) )
local.start()
# Start remote side with swapped TX / RX lines
remote = threading.Thread(target=run_endpoint, args=(remote_side, tx_queue, rx_queue) )
remote.start()

"""
 Main cyclce.
"""
try:
    # Send message from local and remote sides ones per second
    while True:
        remote_side.send( bytearray(b"From remote"))
        local_side.send( bytearray(b"From local"))
        time.sleep(1)
except KeyboardInterrupt:
    print("\nExit")
    stop = True

remote.join()
local.join()
