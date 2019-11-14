/*
    MIT License

    Copyright (c) 2018 - 2019, Alexey Dynda

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <TinyProtocolHd.h>
#include <stdio.h>

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
    /* We do want to use simple checkSum */
    proto.enableCheckSum();
    /* Redirect all protocol communication to Serial0 UART */
//    proto.beginToSerial();
}

void loop()
{
    /* Prepare data you want to send here */
    Tiny::Packet<6> packet;
    packet.put( "HELLO" );
    /* Send packet over UART to other side */
    proto.write(packet);
    proto.run();
}


extern void setup(void);
extern void loop(void);

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

void main_task(void *args)
{
    setup();
    for(;;) {
        loop();
    }
}

extern "C" void app_main(void)
{
    xTaskCreatePinnedToCore(main_task, "mainTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
}
