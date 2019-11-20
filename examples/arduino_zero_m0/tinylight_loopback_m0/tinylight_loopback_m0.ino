/*
 * This example sends back every buffer received over UART.
 *
 * !README!
 * The sketch is developed to perform UART tests between Arduino
 * and PC.
 * 1. Burn this program to Arduino
 * 2. Compile tiny_loopback tool (see tools folder) for your system
 * 3. Connect Arduino TX and RX lines to your PC com port
 * 4. Run tiny_loopback on the PC (use correct port name on your system)
 * 5. tiny_loopback will print the test speed results
 *
 * Also, this example demonstrates how to pass data between 2 systems
 * By default the sketch and tiny_loopback works as 115200 speed.
 */

/* !!! WARNING !!! THIS SKETCH ONLY FOR ARDUINO ZERO/M0 !!! */

#define HAVE_SERIALUSB
#include <TinyProtocol.h>

/* Creating protocol object is simple */
Tiny::ProtoLight  proto;

void setup()
{
    /* No timeout, since we want non-blocking UART operations. */
    SerialUSB.setTimeout(10);
    /* Initialize serial protocol for test purposes */
    SerialUSB.begin(115200);
    /* Lets use 8-bit checksum, available on all platforms */
    proto.enableCheckSum();
    /* Redirect all protocol communication to SerialUSB UART */
    proto.beginToSerialUSB();
}

/* Specify buffer for packets to send and receive */
Tiny::Packet<256> packet;

void loop()
{
    if (SerialUSB.available())
    {
        int len = proto.read( packet );
        if (len > 0)
        {
            /* Send message back */
            proto.write( packet );
        }
    }
}
