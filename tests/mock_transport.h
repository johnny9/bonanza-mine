#ifndef BZM_TESTS_MOCK_TRANSPORT_H
#define BZM_TESTS_MOCK_TRANSPORT_H

#include "bzm/bzm.h"
#include <stddef.h>
#include <stdint.h>

typedef struct mock_transport {
    // capture last tx
    uint8_t last_asic;
    uint8_t last_payload[512];
    size_t  last_len;

    // queued rx data
    uint8_t rx_buf[512];
    size_t  rx_len;
    size_t  rx_off;

    int fail_tx;
    int fail_rx;
} mock_transport_t;

void mock_transport_init(mock_transport_t *m);
void mock_transport_queue_rx(mock_transport_t *m, const uint8_t *data, size_t len);
void mock_transport_bind(bzm_transport_t *io, mock_transport_t *m);

#endif // BZM_TESTS_MOCK_TRANSPORT_H

