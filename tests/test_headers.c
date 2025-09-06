#include "bzm/bzm.h"
#include <assert.h>
#include <stdio.h>

static void test_header16(void) {
    uint16_t h = bzm_header16(0xAB, 0xC);
    uint8_t be[2];
    bzm_u16_be(be, h);
    assert(be[0] == 0xAB);
    assert(be[1] == 0xC0);
}

static void test_header32(void) {
    uint32_t h = bzm_header32(0x12, 0x3, 0x456, 0x78);
    uint8_t be[4];
    bzm_u32_be(be, h);
    assert(be[0] == 0x12);
    // opcode in high nibble of byte 1
    assert((be[1] & 0xF0) == 0x30);
    // engine 0x456 -> 0x0456 spread across bytes 1..2
    // lower nibble of be[1] is high 4 bits of engine
    assert((be[1] & 0x0F) == 0x04);
    assert(be[2] == 0x56);
    assert(be[3] == 0x78);
}

int main(void) {
    test_header16();
    test_header32();
    printf("headers ok\n");
    return 0;
}

