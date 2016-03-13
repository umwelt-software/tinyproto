# Tiny Protocol

Tiny Microcontroller Communication Protocol.


1. Tiny Protocol Packet structure.

Full packet format:
   8    16     any len      16/32   8
 | 7E | UID |  USER DATA  |  FCS  | 7E |


 - 7E is standard packet delimiter (commonly used on layer 2) as defined in HDLC/PPP framing.
 - FCS is standard checksum. Depending on your selection, this is 16-bit or 32-bit. Refer to
   RFC1662 for examples.
 - UID can be used for your own needs. This field is intented for use with higher levels.

Each packet can be sent with simple api command:
    int hdlc_send(SHdlcData *handle, uint16_t *uid, uint8_t *pbuf, int len, uint32_t flags)
And be received via another simple api command: 
    int hdlc_read(SHdlcData *handle, uint16_t *uid, uint8_t *pbuf, int len, uint32_t flags);

uid parameter is optional and can be be simply NULL. In this case, uid parameter is not used,
and the protocol packet looks as follows:
   8     any len      16/32   8
 | 7E |  USER DATA  |  FCS  | 7E |

So, the protocol itself adds a little overhead to the user data: 2 bytes of packet delimiter,
2 bytes FCS field in the simplest case (total 4-bytes). And in the case of full packet format
the protocol adds 8 bytes of overhead.
