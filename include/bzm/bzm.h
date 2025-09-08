// Public API for the Bonanza Mine (bzm) protocol library
#ifndef BZM_BZM_H
#define BZM_BZM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opcodes (4-bit values)
enum {
    BZM_OPCODE_WRITEJOB   = 0x0,
    BZM_OPCODE_READRESULT = 0x1,
    BZM_OPCODE_WRITEREG   = 0x2,
    BZM_OPCODE_READREG    = 0x3,
    BZM_OPCODE_MCASTWRITE = 0x4,
    BZM_OPCODE_DTS_VS     = 0xD,
    BZM_OPCODE_LOOPBACK   = 0xE,
    BZM_OPCODE_NOOP       = 0xF,
};

// Transport abstraction. Implementations must perform the 9-bit 'address mark'
// for the initial address byte and then transmit the provided payload bytes.
typedef struct bzm_transport {
    void *ctx; // user-provided context
    // Transmit: send address-marked 'asic' byte and then 'payload' bytes.
    // Return 0 on success, negative on error.
    int (*tx)(void *ctx, uint8_t asic, const uint8_t *payload, size_t len);
    // Receive exactly 'len' bytes into 'buf' within 'timeout_ms'.
    // Return 0 on success, negative on error or timeout.
    int (*rx)(void *ctx, uint8_t *buf, size_t len, uint32_t timeout_ms);
} bzm_transport_t;

typedef enum bzm_status {
    BZM_OK = 0,
    BZM_EINVAL = -1,
    BZM_EIO = -2,
    BZM_ETIMEOUT = -3,
} bzm_status_t;

// Utility: TAR byte used by READREG
#define BZM_TAR_BYTE ((uint8_t)0x08)

// Header builders (big-endian on wire)
static inline uint16_t bzm_header16(uint8_t asic, uint8_t opcode) {
    return (uint16_t)(((uint16_t)asic << 8) | (((uint16_t)(opcode & 0xF)) << 4));
}

static inline uint32_t bzm_header32(uint8_t asic, uint8_t opcode, uint16_t engine, uint8_t offset) {
    return ((uint32_t)asic << 24) | ((uint32_t)(opcode & 0xF) << 20) |
           ((uint32_t)(engine & 0x0FFF) << 8) | (uint32_t)offset;
}

// Serialize big-endian for fixed-width headers
static inline void bzm_u16_be(uint8_t out[2], uint16_t v) {
    out[0] = (uint8_t)(v >> 8);
    out[1] = (uint8_t)(v & 0xFF);
}

static inline void bzm_u32_be(uint8_t out[4], uint32_t v) {
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)((v >> 16) & 0xFF);
    out[2] = (uint8_t)((v >> 8) & 0xFF);
    out[3] = (uint8_t)(v & 0xFF);
}

// READRESULT parsed structure
typedef struct bzm_result {
    uint16_t engine_id; // 12-bit value in lower bits
    uint8_t  sts;       // lower 4 bits valid; bit3 (0x8) indicates valid result
    uint8_t  nonce[4];  // raw bytes as on wire
    uint8_t  sequence_id;
    uint8_t  time;
} bzm_result_t;

// Command helpers

// WRITEREG: write 1..248 bytes to engine/offset
bzm_status_t bzm_writereg(bzm_transport_t *io, uint8_t asic, uint16_t engine,
                          uint8_t offset, const uint8_t *values, size_t count);

// READREG: read 1,2, or 4 bytes from engine/offset
bzm_status_t bzm_readreg(bzm_transport_t *io, uint8_t asic, uint16_t engine,
                         uint8_t offset, size_t count, uint8_t *out, uint32_t timeout_ms);

// MULTICAST WRITE: engine interpreted as group ID
bzm_status_t bzm_multicast_write(bzm_transport_t *io, uint8_t asic, uint16_t group_id,
                                 uint8_t offset, const uint8_t *values, size_t count);

// WRITEJOB compact packet: midstate[32], merkle_root_residue[4], sequence, job_ctl
bzm_status_t bzm_writejob(bzm_transport_t *io, uint8_t asic, uint16_t engine,
                          const uint8_t midstate[32], const uint8_t merkle_root_residue[4],
                          uint8_t sequence, uint8_t job_ctl);

// READRESULT: fetch result (8 bytes payload)
bzm_status_t bzm_readresult(bzm_transport_t *io, uint8_t asic, bzm_result_t *out, uint32_t timeout_ms);

// DTS/VS READ: read into caller-provided buffer of expected size (gen-dependent)
bzm_status_t bzm_dts_vs_read(bzm_transport_t *io, uint8_t asic, uint8_t *buf, size_t len, uint32_t timeout_ms);

// LOOPBACK: send data, expect echo of length+data; copies echoed data to out
bzm_status_t bzm_loopback(bzm_transport_t *io, uint8_t asic, const uint8_t *data, size_t len,
                          uint8_t tar_or_pad, uint8_t *out, uint32_t timeout_ms);

// NOOP: expect "2ZB"
bzm_status_t bzm_noop(bzm_transport_t *io, uint8_t asic, uint32_t timeout_ms);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BZM_BZM_H
