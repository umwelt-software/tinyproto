/*
 * This simple example listens for commands from UART.
 * This is demonstration of Half Duplex protocol.
 */
#include <TinyProtocol.h>

/* Function to receive incoming messages from remote side */
void onFrameIn(uint8_t *buf, int len)
{
    /* Do what you need with receive data here */
}

/* Half Duplex protocol requires some buffer to store incoming data */
uint8_t buffer[64];

/* Creating Half Duplex protocol object is simple           */
/* Just pass buffer info and callback for incoming messages */
Tiny::ProtoHd  proto(buffer, sizeof(buffer), onFrameIn);



void setup() {
    /* No timeout, since we want non-blocking UART operations */
    Serial.setTimeout(0);
    /* Initialize serial protocol for test purposes */
    Serial.begin(115200);
    /* We do want to use simple checkSum */
    proto.enableCheckSum();
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
}


void loop()
{
    proto.run();
}
