#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _hdlc_handle_t
{
    void *user_data;
    int (*send_tx)(void *user_data, const void *data, int len);
    int (*on_frame_data)(void *user_data, void *data, int len);
    void *rx_buf;
    int rx_buf_size;

    struct
    {
        uint8_t *data;
        int len;
        int (*state)( struct _hdlc_handle_t *handle, uint8_t *data, int len );
        uint8_t escape;
    } rx;
    struct
    {
        uint8_t *data;
        int len;
        uint16_t crc;
        uint8_t escape;
        int (*state)( struct _hdlc_handle_t *handle );
    } tx;
} hdlc_struct_t, *hdlc_handle_t;

hdlc_handle_t hdlc_init( hdlc_struct_t *hdlc_info );

int hdlc_close( hdlc_handle_t handle );

int hdlc_on_rx_data( hdlc_handle_t handle, void *data, int len );

int hdlc_run_tx( hdlc_handle_t handle );

int hdlc_send( hdlc_handle_t handle, void *data, int len );

#ifdef __cplusplus
}
#endif
