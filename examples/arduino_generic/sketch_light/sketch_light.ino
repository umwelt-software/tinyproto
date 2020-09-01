/*
 * This example sends <Hello> packet every second.
 */
#include <TinyProtocol.h>

/* Creating protocol object is simple */
Tiny::ProtoLight  proto;

void setup() {
    /* No timeout, since we want non-blocking UART operations */
    Serial.setTimeout(0);
    /* Initialize serial protocol for test purposes */
    Serial.begin(115200);
    /* We do not want to use crc */
    proto.enableCheckSum();
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
}

/* Specify buffer for packets to send and receive */
Tiny::Packet<32> inPacket;

void loop()
{
    Tiny::Packet<32> outPacket;
    /* Prepare data you want to send here - null-terminated string "DATA" */
    outPacket.put("DATA");
    /* Send packet over UART to other side */
    proto.write(outPacket);

    unsigned long start = millis();
    /* read answer from remote side */
    int len;
    do
    {
        len = proto.read(inPacket);
        /* Wait 1000ms for any answer from remote side */
        if ((unsigned long)(millis() - start) >= 1000)
        {
            break;
        }
    } while (len <= 0);
    if (len > 0)
    {
        /* Process response in inPacket from remote side here (len bytes) */
    }
}
