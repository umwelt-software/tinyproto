/*
 * This simple example listens for commands from UART.
 */
#include <TinyProtocol.h>

/* Creating protocol object is simple */
Tiny::Proto  proto;



void setup() {
    /* No timeout, since we want non-blocking UART operations */
    Serial.setTimeout(0);
    /* Initialize serial port */
    Serial.begin(9600);
    /* Enable checksum to detect UART errors. Nano supports only 8-bit checksum. */
    proto.enableCheckSum();
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
}

/* Specify buffer for processing data being received or sent.
 * These buffers must have enough size to place biggest possible packet.
 * Use Tiny::Packet wrappers if you want to have simple way to put data to these buffers.
 */
char g_inBuf[32];
char g_outBuf[32];

// Tiny::Packet outPacket(g_outBuf, sizeof(g_outBuf));
// Tiny::Packet inPacket(g_inBuf, sizeof(g_inBuf));


void loop()
{
    /* Check if some data are waiting for reading in UART */
    int len;
    do {
        len = proto.read(g_inBuf, sizeof(g_inBuf), TINY_FLAG_NO_WAIT);
    } while (len == TINY_ERR_OUT_OF_SYNC);
    if (len > 0)
    {
        /* Ok, we received something from other side - len bytes */
        /* Process data here. And send response back if you need. */
        g_outBuf[0]='O'; g_outBuf[1]='K';
        proto.write(g_outBuf, 2, TINY_FLAG_WAIT_FOREVER);
    }
    /* Do what you want here. The protocol will not block this code
       since it is non-blocking.
     */
}
