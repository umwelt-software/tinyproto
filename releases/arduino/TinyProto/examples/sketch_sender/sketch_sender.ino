/*
 * This example sends <Hello> packet every second.
 */
#include <TinyProtocol.h>

/* Creating protocol object is simple */
Tiny::Proto  proto;

void setup() {
    /* No timeout, since we want non-blocking UART operations */
    Serial.setTimeout(0);
    /* Initialize serial protocol for test purposes */
    Serial.begin(9600);
    /* We do not want to use crc */
    proto.enableCheckSum();
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
}

/* Specify buffer for packets to send and receive */
char g_inBuf[32];
char g_outBuf[32];

void loop()
{
    /* Prepare data you want to send here */
    g_outBuf[0] = 'D'; g_outBuf[1] = 'A'; g_outBuf[2] = 'T'; g_outBuf[3] = 'A';

    /* Send packet over UART to other side */
    proto.write(g_outBuf, 4, TINY_FLAG_WAIT_FOREVER);

    unsigned long start = millis();
    /* read answer from remote side */
    int len;
    do
    {
        len = proto.read(g_inBuf, sizeof(g_inBuf), TINY_FLAG_NO_WAIT);
        /* Wait 1000ms for any answer from remote side */
        if (millis() - start >= 1000)
        {
            break;
        }
    } while (len < 0);
    if (len > 0)
    {
        /* Process response from remote side here (len bytes) */
    }
}
