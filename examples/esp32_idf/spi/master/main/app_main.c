/* SPI Slave example, sender (uses SPI master driver)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/igmp.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "soc/rtc_periph.h"
#include "esp32/rom/cache.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_spi_flash.h"

#include "driver/gpio.h"
#include "esp_intr_alloc.h"

#include "proto/hdlc/high_level/hdlc.h"

/*
SPI sender (master) example.

This example is supposed to work together with the SPI receiver. It uses the standard SPI pins (MISO, MOSI, SCLK, CS) to
transmit data over in a full-duplex fashion, that is, while the master puts data on the MOSI pin, the slave puts its own
data on the MISO pin.

This example uses one extra pin: GPIO_HANDSHAKE is used as a handshake pin. The slave makes this pin high as soon as it
is ready to receive/send data. This code connects this line to a GPIO interrupt which gives the s_slaveReady semaphore.
The main task waits for this semaphore to be given before queueing a transmission.
*/

/*
Pins in use. The SPI Master can use the GPIO mux, so feel free to change these if needed.
*/
#define GPIO_HANDSHAKE 2
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 15
#define GPIO_CS 14
#define SPI_TRANSACTION_BLOCK_SIZE 16

// The semaphore indicating the slave is ready to receive stuff.
static xQueueHandle s_slaveReady;

hdlc_struct_t s_hdlc = {0};
uint8_t s_hdlc_rx_buffer[128];
spi_device_handle_t s_handle;

/*
This ISR is called when the handshake line goes high.
*/
static void IRAM_ATTR gpio_handshake_isr_handler(void *arg)
{
    // Sometimes due to interference or ringing or something, we get two irqs after eachother. This is solved by
    // looking at the time between interrupts and refusing any interrupt too close to another one.
    /*    static uint32_t lasthandshaketime;
        uint32_t currtime=xthal_get_ccount();
        uint32_t diff=currtime-lasthandshaketime;
        if (diff<240000) return; //ignore everything <1ms after an earlier irq
        lasthandshaketime=currtime;*/

    // Give the semaphore.
    BaseType_t mustYield = false;
    xSemaphoreGiveFromISR(s_slaveReady, &mustYield);
    if ( mustYield )
        portYIELD_FROM_ISR();
}

void init_spi()
{
    esp_err_t ret;

    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {.mosi_io_num = GPIO_MOSI,
                               .miso_io_num = GPIO_MISO,
                               .sclk_io_num = GPIO_SCLK,
                               .quadwp_io_num = -1,
                               .quadhd_io_num = -1};

    // Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 1000000, // Higher speed can cause SPI issues on slave side
        .duty_cycle_pos = 128,     // 50% duty cycle
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .cs_ena_posttrans = 3, // Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit
                               // when CS has less propagation delay than CLK
        .queue_size = 1};

    // GPIO config for the handshake line.
    gpio_config_t io_conf = {.intr_type = GPIO_PIN_INTR_POSEDGE,
                             .mode = GPIO_MODE_INPUT,
                             .pull_up_en = 0,
                             .pull_down_en = 1,
                             .pin_bit_mask = (1 << GPIO_HANDSHAKE)};

    // Create the semaphore.
    s_slaveReady = xSemaphoreCreateBinary();

    // Set up handshake line interrupt.
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_set_intr_type(GPIO_HANDSHAKE, GPIO_PIN_INTR_POSEDGE);
    gpio_isr_handler_add(GPIO_HANDSHAKE, gpio_handshake_isr_handler, NULL);

    // Initialize the SPI bus and add the device we want to send stuff to.
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret == ESP_OK);
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &s_handle);
    assert(ret == ESP_OK);
    // Assume the slave is ready for the first transmission: if the slave started up before us, we will not detect
    // positive edge on the handshake line.
    xSemaphoreGive(s_slaveReady);
}

void deinit_spi()
{
    esp_err_t ret;
    ret = spi_bus_remove_device(s_handle);
    assert(ret == ESP_OK);
}

int on_receive(void *user_data, uint8_t addr, void *data, int size)
{
    static uint8_t last_index = 0;
    uint8_t index = *((uint8_t *)data);
    last_index++;
    if ( index != last_index )
    {
        fprintf(stderr, "!!! Missed frame(s)\n");
    }
    fprintf(stderr, "<< Received: [%02X] %.*s\n", index, size - 1, ((char *)data) + 1);
    last_index = index;
    return 0;
}

int on_send(void *user_data, uint8_t addr, const void *data, int size)
{
    uint8_t index = *((const uint8_t *)data);
    fprintf(stderr, ">> Sent: [%02X] %.*s\n", index, size - 1, ((const char *)data) + 1);
    return 0;
}

void communication_task()
{
    char sendbuf[SPI_TRANSACTION_BLOCK_SIZE] = {0};
    char recvbuf[SPI_TRANSACTION_BLOCK_SIZE] = {0};
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    while ( 1 )
    {
        // Fill tx buffer before SPI transaction with some inter-frame data
        memset(sendbuf, TINY_HDLC_FILL_BYTE, sizeof(sendbuf));
        hdlc_get_tx_data(&s_hdlc, sendbuf, sizeof(sendbuf));

        t.length = sizeof(sendbuf) * 8;
        t.tx_buffer = sendbuf;
        t.rx_buffer = recvbuf;

        //        for(int i=0; i<sizeof(sendbuf); i++)
        //            fprintf(stderr, "0x%02X('%c') ", sendbuf[i], sendbuf[i] );
        //        fprintf( stderr, "\n" );

        // Wait for slave to be ready for next byte before sending
        xSemaphoreTake(s_slaveReady, portMAX_DELAY); // Wait until slave is ready
        esp_err_t ret = spi_device_transmit(s_handle, &t);
        assert(ret == ESP_OK);

        //        for(int i=0; i<sizeof(recvbuf); i++)
        //            fprintf(stderr, "0x%02X('%c') ", recvbuf[i], recvbuf[i] );
        //        fprintf( stderr, "\n" );
        // Parse all data received over SPI
        int len = sizeof(recvbuf);
        while ( len )
        {
            len -= hdlc_run_rx(&s_hdlc, recvbuf + sizeof(recvbuf) - len, len, NULL);
        }
    }
}

// Main application
void app_main()
{
    init_spi();

    s_hdlc.crc_type = HDLC_CRC_16;
    s_hdlc.on_frame_read = on_receive;
    s_hdlc.on_frame_sent = on_send;
    s_hdlc.rx_buf = s_hdlc_rx_buffer;
    s_hdlc.rx_buf_size = sizeof(s_hdlc_rx_buffer);

    hdlc_init(&s_hdlc);

    xTaskCreate(communication_task, "protocol_task", 2096, NULL, 1, NULL);

    char message[128] = "\0This is protocol message from master";

    while ( 1 )
    {
        tiny_sleep(1000); // Send message every 1 second
        message[0]++;
        hdlc_send(&s_hdlc, message, 1 + strlen(message + 1), 0);
    }

    hdlc_close(&s_hdlc);
    // Never reached.
    deinit_spi();
}
