/*
 * This example sends back every packet received over UART. 
 * To test this example, just compile it and upload to Arduino controller.
 * Open Serial Monitor in Arduino IDE. send anything typing data around 
 * '~' chars (for example, ~Welcome~ ), and Arduino will send back the packet to you.
 * 
 * TinyProtocol uses some special frame format, that means that it will ignore not valid
 * chars received over UART. Each packet should be in the following format:
 * ~[UID]DATA[FCS]~
 * UID and FCS field are optional. In the example below, they are disabled. Nano version
 * of Tiny protocol supports only simple 1-byte Checksum field. For CRC-16/CRC-32 use
 * Full versions of Tiny protocol.
 */
#include <TinyProtocol.h>

/* Creating protocol object is simple */
Tiny::ProtoLight  proto;

void setup() {
    /* No timeout, since we want non-blocking UART operations. */
    Serial.setTimeout(0);
    /* Initialize serial protocol for test purposes */
    Serial.begin(9600);
    /* We do not want to use crc */
    proto.disableCrc();
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
}

/* Create special class, which simplifies the work with buffer */
Tiny::Packet<16> g_packet;

void loop()
{
    /* Check if some data are waiting for reading in UART */
    /* If there is no delay in loop() cycle, there is no need to check for OUT_OF_SYNC error */
    int len = proto.read( g_packet );
    /* Check if we received something with no error */
    if (len > 0)
    {
        /* Send received packet back to UART (echo) */
        proto.write( g_packet );
    }
}
