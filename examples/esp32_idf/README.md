## Examples for ESP32

This folder contains 2 examples. Please follow instructions for each example and prepare 2 boards:

 * tinyfd_generator (Generates fixed frames, sends to remote side, and receives back)
 * tinyfd_loopback (Receives frames, and sends them back to sender)

These examples use UART1 via D4 (TX), D5 (RX) pins

You can use single ESP32 Dev board with PC tool.

## Registered full duplex speed with 2 ESP32 Dev boards over UART (115200)

Registered full duplex speed with 2 ESP32 Dev boards over UART (running at 115200) is logged below.
So, Tiny Protocol shows high channel utilization. However, payload transfer speed cannot be very close
to UART baud rate because of start/stop bits, and hdlc full duplex overhead, which is usually
from 4 to 20 bytes for each payload buffer being transferred.

```.txt
Registered TX speed: payload 76921 bps, total 113120 bps
Registered RX speed: payload 76377 bps, total 112320 bps
```
