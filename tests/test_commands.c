// Unity-based tests
#include "bzm/bzm.h"
#include "mock_transport.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_writereg(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    uint8_t data[3] = {0xDE, 0xAD, 0xBE};
    bzm_status_t st = bzm_writereg(&io, 0x01, 0x234, 0x56, data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(BZM_OK, st);
    TEST_ASSERT_EQUAL_HEX8(0x01, m.last_asic);
    // header (3 bytes) + count + data
    TEST_ASSERT_EQUAL_size_t(3 + 1 + 3, m.last_len);
    // opcode in high nibble (0x2) and engine high nibble 0x2
    TEST_ASSERT_EQUAL_HEX8(0x20, (uint8_t)(m.last_payload[0] & 0xF0));
    TEST_ASSERT_EQUAL_HEX8(0x02, (uint8_t)(m.last_payload[0] & 0x0F));
    TEST_ASSERT_EQUAL_HEX8(0x34, m.last_payload[1]);
    TEST_ASSERT_EQUAL_HEX8(0x56, m.last_payload[2]);
    TEST_ASSERT_EQUAL_HEX8(2, m.last_payload[3]);
    TEST_ASSERT_EQUAL_HEX8(0xDE, m.last_payload[4]);
}

static void test_readreg(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    // queue response: echo 1 header byte + 4 data bytes
    uint8_t resp[] = {0xAA, 0x11, 0x22, 0x33, 0x44};
    mock_transport_queue_rx(&m, resp, sizeof(resp));
    uint8_t out[4] = {0};
    bzm_status_t st = bzm_readreg(&io, 0x02, 0xFFF, 0x10, 4, out, 100);
    TEST_ASSERT_EQUAL_INT(BZM_OK, st);
    TEST_ASSERT_EQUAL_HEX8(0x02, m.last_asic);
    TEST_ASSERT_EQUAL_size_t(3 + 1 + 1, m.last_len);
    TEST_ASSERT_EQUAL_HEX8(0x30, (uint8_t)(m.last_payload[0] & 0xF0));
    TEST_ASSERT_EQUAL_HEX8(0x0F, (uint8_t)(m.last_payload[0] & 0x0F));
    TEST_ASSERT_EQUAL_HEX8(0xFF, m.last_payload[1]);
    TEST_ASSERT_EQUAL_HEX8(0x10, m.last_payload[2]);
    TEST_ASSERT_EQUAL_HEX8(3, m.last_payload[3]);
    TEST_ASSERT_EQUAL_HEX8(BZM_TAR_BYTE, m.last_payload[4]);
    uint8_t exp[4] = {0x11,0x22,0x33,0x44};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 4);
}

static void test_noop(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    uint8_t resp[] = {'2','Z','B'};
    mock_transport_queue_rx(&m, resp, sizeof(resp));
    bzm_status_t st = bzm_noop(&io, 0x03, 100);
    TEST_ASSERT_EQUAL_INT(BZM_OK, st);
    TEST_ASSERT_EQUAL_HEX8(0x03, m.last_asic);
    TEST_ASSERT_EQUAL_size_t(1, m.last_len);
    TEST_ASSERT_EQUAL_HEX8(0xF0, m.last_payload[0]);
}

static void test_readresult(void) {
    mock_transport_t m; mock_transport_init(&m);
    bzm_transport_t io; mock_transport_bind(&io, &m);
    // engine_id:0x123, sts:0x8 => 16-bit be: (0x123<<4)|0x8 = 0x1238 => bytes 0x12,0x38
    uint8_t resp[] = {0x12, 0x38, 0xAA, 0xBB, 0xCC, 0xDD, 0xFE, 0x10};
    mock_transport_queue_rx(&m, resp, sizeof(resp));
    bzm_result_t r;
    bzm_status_t st = bzm_readresult(&io, 0x04, &r, 100);
    TEST_ASSERT_EQUAL_INT(BZM_OK, st);
    TEST_ASSERT_EQUAL_HEX8(0x04, m.last_asic);
    TEST_ASSERT_EQUAL_size_t(1, m.last_len);
    TEST_ASSERT_EQUAL_HEX8(0x10, m.last_payload[0]);
    TEST_ASSERT_EQUAL_HEX16(0x0123, r.engine_id);
    TEST_ASSERT_EQUAL_HEX8(0x08, r.sts);
    TEST_ASSERT_EQUAL_HEX8(0xAA, r.nonce[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, r.sequence_id);
    TEST_ASSERT_EQUAL_HEX8(0x10, r.time);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_writereg);
    RUN_TEST(test_readreg);
    RUN_TEST(test_noop);
    RUN_TEST(test_readresult);
    return UNITY_END();
}
