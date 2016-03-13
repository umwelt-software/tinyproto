/*
    Copyright 2016 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fakewire.h"
#include "tiny_layer2.h"


/* size of HDLC protocol buffer */
#define PROTO_BUF_SIZE 256

fake_wire_t wire1;
fake_wire_t wire2;
fake_line_t wirelines[2];


static pthread_t thread1;
static pthread_t thread2;
static uint8_t stop_flag = 0;

void *receiveThread(void *arg)
{
    uint16_t n_sent = 0;
    uint16_t n_received = 0;
    STinyData channel = {0};
    char outbuf[PROTO_BUF_SIZE];
    char inbuf[PROTO_BUF_SIZE];
    int side = (uint64_t)arg;
    int result;

    printf("Hi\n");

    if (tiny_init(&channel, fakeWireWrite, fakeWireRead, &wirelines[side]) < 0)
    {
        printf("Failed to initialize HDLC protocol\n");
        return (void *)-1;
    }
    while ((n_sent < 32000) && (!stop_flag))
    {
        snprintf(outbuf, PROTO_BUF_SIZE - 1, "This is frame Number %i (stream %i)", n_sent, side);
        result = tiny_send(&channel, &n_sent, (uint8_t *)outbuf, strlen(outbuf) + 1, TINY_FLAG_NO_WAIT);
        if (result == strlen(outbuf) + 1)
        {
            printf("[STREAM: %i, SENT]: [%u] %s\n", side, n_sent, outbuf);
            n_sent++;
        }
        result = tiny_read(&channel, &n_received, (uint8_t *)inbuf, PROTO_BUF_SIZE, TINY_FLAG_NO_WAIT);
        if (result > 0)
        {
            printf("[STREAM: %i, RECEIVED]: [%u] %s\n", side, n_received, inbuf);
            //n_received++;
        }
    }

    printf("[STREAM: %i] oosync_bytes: %i\n", side, channel.stat.oosyncBytes);
    printf("[STREAM: %i] frames_sent: %i\n", side, channel.stat.framesSent);
    printf("[STREAM: %i] frames_received: %i\n", side, channel.stat.framesReceived);
    printf("[STREAM: %i] frames_broken: %i\n", side, channel.stat.framesBroken);

    /* Stop friend stream as we're not going to fetch data from the channel anymore */
    stop_flag = 1;

    tiny_close(&channel);
    return 0;
}


int initHardware()
{
    if (fakeWireInit(&wirelines[0], &wire1, &wire2, 0) < 0)
    {
        printf("Failed to init hardware\n");
        return -1;
    }
    if (fakeWireInit(&wirelines[1], &wire2, &wire1, 0) < 0)
    {
        fakeWireClose(&wirelines[0]);
        printf("Failed to init hardware\n");
        return -1;
    }
    return 0;
}


int freeHardware()
{
    fakeWireClose(&wirelines[0]);
    fakeWireClose(&wirelines[1]);
    return 0;
}


int runThreads()
{
    int result;
    result = pthread_create( &thread1, NULL, receiveThread, (void *)0);
    if (result)
    {
        printf("Failed to create Thread1\n");
        return -1;
    }
    result = pthread_create( &thread1, NULL, receiveThread, (void *)1);
    if (result)
    {
        printf("Failed to create Thread1");
        return -1;
    }
    return 0;
}

void waitThreads()
{
    void *result;
    pthread_join(thread1, &result);
    pthread_join(thread2, &result);
}



int main(int argc, char *argv[])
{
    printf("[DONE]\n");
    if (initHardware() < 0)
        return -1;
    printf("[DONE]\n");
    if (runThreads() < 0)
    {
        freeHardware();
        return -1;
    }

    waitThreads();
    freeHardware();
    printf("[DONE]\n");
    return 0;
}
