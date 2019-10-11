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
#include <TinyProtocol.h>

void onReceive(Tiny::IPacket &pkt);

/* Creating protocol object is simple. Lets allocate 512 bytes for it. *
 * In this case, 512 byte buffer is used to store internal state and.  *
 * 4 buffers for outgoing and 1 buffer for incoming frames.            */
Tiny::ProtoFd<512>  proto(onReceive);

void onReceive(Tiny::IPacket &pkt)
{
    if ( proto.write(pkt) == TINY_ERR_TIMEOUT )
    {
        // Do what you need to do if there is no place to put new frame to.
        // But never use blocking operations inside callback
    }
}

void setup()
{
    /* No timeout, since we want non-blocking UART operations. */
    Serial.setTimeout(10);
    /* Initialize serial protocol for test purposes */
    Serial.begin(115200);
    /* Redirect all protocol communication to Serial0 UART */
    proto.beginToSerial();
}

void loop()
{
    if (Serial.available())
    {
        proto.run_rx();
    }
    proto.run_tx();
}
