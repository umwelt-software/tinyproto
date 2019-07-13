#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    TX_STATE_IDLE,
    TX_STATE_SEND_DATA,
    TX_STATE_SEND_CRC,
    TX_STATE_END,
} tiny_tx_state_t;

typedef struct _hdlc_handle_t
{
    void *user_data;
    int (*send_tx)(void *user_data, const void *data, int len);
    struct
    {
        uint8_t *data;
        int len;
        uint16_t crc;
        uint8_t escape;
        int (*state)( struct _hdlc_handle_t *handle );
    } tx;
} hdlc_struct_t, *hdlc_handle_t;

int hdlc_init( hdlc_handle_t handle,
               int (*send_tx)(void *user_data, const void *data, int len),
               void *user_data );

int hdlc_on_rx_data( hdlc_handle_t handle, void *data, int len );

int hdlc_run_tx( hdlc_handle_t handle );

int hdlc_send( hdlc_handle_t handle, void *data, int len );

#ifdef __cplusplus
}
#endif
