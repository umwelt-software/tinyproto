/*
 * This example sends back every packet received over UART. 
 * To test this example, just compile it and upload to Arduino controller.
 * Open Serial Monitor in Arduino IDE. send anything typing data around 
 * '~' chars (for example, ~Welcome~ ), and Arduino will send back the packet to you.
 * 
 * TinyProtocol uses some special frame format, that means that it will ignore not valid
 * chars received over UART. Each packet should be in the following format:
 * ~DATA~
 * UID and FCS field are optional. In the example below, they are disabled. Nano version
 * of Tiny protocol supports only simple 1-byte Checksum field. For CRC-16/CRC-32 use
 * Full versions of Tiny protocol.
 */
#include <TinyLightProtocol.h>

/* Creating protocol object is simple */
Tiny::ProtoLight  proto;

void setup()
{
    /* No timeout, since we want non-blocking UART operations. */
    Serial.setTimeout(10);
    /* Initialize serial protocol for test purposes */
    Serial.begin(115200);
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
}

/* Specify buffer for packets to send and receive */
char g_buf[256];

void loop()
{
    if (Serial.available())
    {
        int len = proto.read( g_buf, sizeof(g_buf) );
        /* If we receive valid message */
        if (len > 0)
        {
            /* Send message back */
            proto.write( g_buf, len );
        }
    }
}
