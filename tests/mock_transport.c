#include "mock_transport.h"
#include <string.h>

static int mock_tx(void *ctx, uint8_t asic, const uint8_t *payload, size_t len) {
    mock_transport_t *m = (mock_transport_t *)ctx;
    if (m->fail_tx) return -1;
    m->last_asic = asic;
    if (len > sizeof(m->last_payload)) len = sizeof(m->last_payload);
    memcpy(m->last_payload, payload, len);
    m->last_len = len;
    return 0;
}

static int mock_rx(void *ctx, uint8_t *buf, size_t len, uint32_t timeout_ms) {
    (void)timeout_ms;
    mock_transport_t *m = (mock_transport_t *)ctx;
    if (m->fail_rx) return -1;
    if (m->rx_off + len > m->rx_len) return -1;
    memcpy(buf, &m->rx_buf[m->rx_off], len);
    m->rx_off += len;
    return 0;
}

void mock_transport_init(mock_transport_t *m) {
    memset(m, 0, sizeof(*m));
}

void mock_transport_queue_rx(mock_transport_t *m, const uint8_t *data, size_t len) {
    if (len > sizeof(m->rx_buf)) len = sizeof(m->rx_buf);
    memcpy(m->rx_buf, data, len);
    m->rx_len = len;
    m->rx_off = 0;
}

void mock_transport_bind(bzm_transport_t *io, mock_transport_t *m) {
    io->ctx = m;
    io->tx = mock_tx;
    io->rx = mock_rx;
}

