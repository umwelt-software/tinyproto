internal packet representation on the levels higher than hdlc.

struct
{
    uint8_t *start;  ///< This is pointer to the buffer for the whole packet
    uint8_t *head;   ///< This is pointer to the first byte of user payload data
    uint8_t *tail;   ///< This is pointer to the first byte after user payload
    uint8_t *end;    ///< This is pointer to the end of the buffer
} s_tiny_packet_t;

For packets being transmitted on the top level these fields are set as:
  * head = start + 8
  * tail = head + user_data_size

Each lower level adds specific information by moving head and tail pointers.



