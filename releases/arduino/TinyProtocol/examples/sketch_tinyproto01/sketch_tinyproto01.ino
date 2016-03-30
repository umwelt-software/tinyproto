/*
 * This example sends <Hello> packet every second.
 * If some packet is received from remote side, it echos content back to the sender
 */
#include <TinyProtocol.h>

const int PIN_LED = 13;

/* Creating protocol object is simple */
Tiny::Proto  proto;

void setup() {
    pinMode(PIN_LED, OUTPUT);
    /* No timeout, since we want non-blocking UART operations */
    Serial.setTimeout(0);
    /* Initialize serial protocol for test purposes */
    Serial.begin(9600);
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
    /* We do not want to use crc */
    proto.disableCrc();
}

/* Specify buffer for packets to send and receive */
char g_inBuf[16];
char g_outBuf[16];

/* Create special class, which simplifies the work with buffer */
Tiny::Packet outPacket(g_outBuf, sizeof(g_outBuf));
Tiny::Packet inPacket(g_inBuf, sizeof(g_inBuf));


void loop()
{
    /* Create simple packet, containing only text */
    outPacket.clear();
    outPacket.put("\nHello\n");

    /* Send packet over UART to other side */
    proto.write(outPacket, TINY_FLAG_WAIT_FOREVER);

    /* Check if some data are waiting for reading in UART */
    /* If there is no delay in loop() cycle, there is no need to check for OUT_OF_SYNC error */
    int len;
    do {
        len = proto.read(inPacket, TINY_FLAG_NO_WAIT);
    } while (len == TINY_ERR_OUT_OF_SYNC);
    if (len > 0)
    {
        /* Send received packet back to UART (echo) */
        proto.write(inPacket, TINY_FLAG_WAIT_FOREVER);
    }
    delay(985);
    digitalWrite(PIN_LED, HIGH);
    delay(15);
    digitalWrite(PIN_LED, LOW);
}
