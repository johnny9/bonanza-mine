#include "bzm/bzm.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_header32(void) {
    uint32_t h = bzm_header32(0x12, 0x3, 0x456, 0x78);
    uint8_t be[4];
    be[0] = (uint8_t)(h >> 24);
    be[1] = (uint8_t)(h >> 16);
    be[2] = (uint8_t)(h >> 8);
    be[3] = (uint8_t)h;
    TEST_ASSERT_EQUAL_HEX8(0x12, be[0]);
    // opcode in high nibble of byte 1
    TEST_ASSERT_BITS_HIGH(0x30, be[1] & 0xF0);
    // engine 0x456 -> 0x0456 spread across bytes 1..2
    TEST_ASSERT_EQUAL_HEX8(0x04, be[1] & 0x0F);
    TEST_ASSERT_EQUAL_HEX8(0x56, be[2]);
    TEST_ASSERT_EQUAL_HEX8(0x78, be[3]);
}

static void test_opcode_payload_byte(void) {
    TEST_ASSERT_EQUAL_HEX8(0x10, (uint8_t)(BZM_OPCODE_READRESULT << 4));
    TEST_ASSERT_EQUAL_HEX8(0xD0, (uint8_t)(BZM_OPCODE_DTS_VS << 4));
    TEST_ASSERT_EQUAL_HEX8(0xF0, (uint8_t)(BZM_OPCODE_NOOP << 4));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_header32);
    RUN_TEST(test_opcode_payload_byte);
    return UNITY_END();
}
