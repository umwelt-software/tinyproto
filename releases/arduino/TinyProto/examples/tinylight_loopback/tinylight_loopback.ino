/*
 * This example sends back every buffer received over UART.
 *
 * !README!
 * The sketch is developed to perform UART tests between Arduino
 * and PC.
 * 1. Burn this program to Arduino
 * 2. Compile sperf tool (see tools folder) for your system
 * 3. Connect Arduino TX and RX lines to your PC com port
 * 4. Run sperf on the PC (use correct port name on your system):
 *      sperf /dev/ttyUSB0
 *      sperf COM1
 * 5. sperf will print the test speed results
 *
 * Also, this example demonstrates how to pass data between 2 systems
 * By default the sketch and sperf works as 115200 speed.
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
