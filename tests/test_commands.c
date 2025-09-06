#include "bzm/bzm.h"
#include "mock_transport.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_writereg(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    uint8_t data[3] = {0xDE, 0xAD, 0xBE};
    bzm_status_t st = bzm_writereg(&io, 0x01, 0x234, 0x56, data, sizeof(data));
    assert(st == BZM_OK);
    // header32 + count + data
    assert(m.last_len == 4 + 1 + 3);
    assert(m.last_payload[0] == 0x01);
    assert(m.last_payload[1] == 0x20 | 0x30); // opcode in high nibble (0x2)
    assert((m.last_payload[1] & 0xF0) == 0x20);
    assert((m.last_payload[1] & 0x0F) == 0x02);
    assert(m.last_payload[2] == 0x34);
    assert(m.last_payload[3] == 0x56);
    assert(m.last_payload[4] == 2);
    assert(m.last_payload[5] == 0xDE);
}

static void test_readreg(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    // queue response: echo 2 bytes + 4 data bytes
    uint8_t resp[] = {0xAA, 0xBB, 0x11, 0x22, 0x33, 0x44};
    mock_transport_queue_rx(&m, resp, sizeof(resp));
    uint8_t out[4] = {0};
    bzm_status_t st = bzm_readreg(&io, 0x02, 0xFFF, 0x10, 4, out, 100);
    assert(st == BZM_OK);
    assert(out[0] == 0x11 && out[1] == 0x22 && out[2] == 0x33 && out[3] == 0x44);
}

static void test_noop(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    uint8_t resp[] = {'2','Z','B'};
    mock_transport_queue_rx(&m, resp, sizeof(resp));
    bzm_status_t st = bzm_noop(&io, 0x03, 100);
    assert(st == BZM_OK);
}

static void test_readresult(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    // engine_id:0x123, sts:0x8 => 16-bit be: (0x123<<4)|0x8 = 0x1238 => bytes 0x12,0x38
    uint8_t resp[] = {0x12, 0x38, 0xAA, 0xBB, 0xCC, 0xDD, 0xFE, 0x10};
    mock_transport_queue_rx(&m, resp, sizeof(resp));
    bzm_result_t r;
    bzm_status_t st = bzm_readresult(&io, 0x04, &r, 100);
    assert(st == BZM_OK);
    assert(r.engine_id == 0x123);
    assert(r.sts == 0x8);
    assert(r.nonce[0] == 0xAA && r.sequence_id == 0xFE && r.time == 0x10);
}

int main(void) {
    test_writereg();
    test_readreg();
    test_noop();
    test_readresult();
    printf("commands ok\n");
    return 0;
}
