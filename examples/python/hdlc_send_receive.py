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

p = tinyproto.Hdlc()
def on_read(a):
    print("Received bytes: " + ','.join( [ "{:#x}".format(x) for x in a ] ) )

def on_send(a):
    print("Sent bytes: " + ','.join( [ "{:#x}".format(x) for x in a ] ) )

# setup protocol
p.on_read = on_read
p.on_send = on_send
p.begin()

# provide rx bytes to the protocol
p.rx( bytearray([ 0x7E, 0xFF, 0x3F, 0xF3, 0x39, 0x7E  ]) )

# Let's try to send single byte frame 0x65
p.put( bytearray( [0x65] ) )
result = p.tx()

print("We need to put this to tx hardware channel: ", ','.join( ["{:#x}".format(x) for x in result ]) )
