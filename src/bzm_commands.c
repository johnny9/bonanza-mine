#include "bzm/bzm.h"
#include <string.h>

// Internal helpers
static void write_u16_be(uint8_t out[2], uint16_t v) {
    out[0] = (uint8_t)(v >> 8);
    out[1] = (uint8_t)(v & 0xFF);
}

static void write_u32_be(uint8_t out[4], uint32_t v) {
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)((v >> 16) & 0xFF);
    out[2] = (uint8_t)((v >> 8) & 0xFF);
    out[3] = (uint8_t)(v & 0xFF);
}

static bzm_status_t tx_payload(bzm_transport_t *io, uint8_t asic, const uint8_t *payload, size_t len) {
    if (!io || !io->tx) return BZM_EINVAL;
    int rc = io->tx(io->ctx, asic, payload, len);
    return (rc == 0) ? BZM_OK : BZM_EIO;
}

static bzm_status_t rx_exact(bzm_transport_t *io, uint8_t *buf, size_t len, uint32_t timeout_ms) {
    if (!io || !io->rx) return BZM_EINVAL;
    int rc = io->rx(io->ctx, buf, len, timeout_ms);
    if (rc == 0) return BZM_OK;
    if (rc < 0) return (rc == BZM_ETIMEOUT) ? BZM_ETIMEOUT : BZM_EIO;
    return BZM_EIO;
}

bzm_status_t bzm_writereg(bzm_transport_t *io, uint8_t asic, uint16_t engine,
                          uint8_t offset, const uint8_t *values, size_t count) {
    if (!io || !values) return BZM_EINVAL;
    if (count < 1 || count > 248) return BZM_EINVAL;

    uint8_t buf[4 + 1 + 248];
    uint32_t h = bzm_header32(asic, BZM_OPCODE_WRITEREG, engine, offset);
    write_u32_be(buf, h);
    buf[4] = (uint8_t)(count - 1);
    memcpy(&buf[5], values, count);
    return tx_payload(io, asic, buf, 5 + count);
}

bzm_status_t bzm_readreg(bzm_transport_t *io, uint8_t asic, uint16_t engine,
                         uint8_t offset, size_t count, uint8_t *out, uint32_t timeout_ms) {
    if (!io || !out) return BZM_EINVAL;
    if (!(count == 1 || count == 2 || count == 4)) return BZM_EINVAL;

    uint8_t req[4 + 1 + 1];
    uint32_t h = bzm_header32(asic, BZM_OPCODE_READREG, engine, offset);
    write_u32_be(req, h);
    req[4] = (uint8_t)(count - 1);
    req[5] = BZM_TAR_BYTE;
    bzm_status_t st = tx_payload(io, asic, req, sizeof(req));
    if (st != BZM_OK) return st;

    // Response: echo header (2 bytes) then count bytes of data
    uint8_t tmp[2 + 4];
    st = rx_exact(io, tmp, (size_t)2 + count, timeout_ms);
    if (st != BZM_OK) return st;
    memcpy(out, &tmp[2], count);
    return BZM_OK;
}

bzm_status_t bzm_multicast_write(bzm_transport_t *io, uint8_t asic, uint16_t group_id,
                                 uint8_t offset, const uint8_t *values, size_t count) {
    if (!io || !values) return BZM_EINVAL;
    if (count < 1 || count > 248) return BZM_EINVAL;

    uint8_t buf[4 + 1 + 248];
    uint32_t h = bzm_header32(asic, BZM_OPCODE_MCASTWRITE, group_id, offset);
    write_u32_be(buf, h);
    buf[4] = (uint8_t)(count - 1);
    memcpy(&buf[5], values, count);
    return tx_payload(io, asic, buf, 5 + count);
}

bzm_status_t bzm_writejob(bzm_transport_t *io, uint8_t asic, uint16_t engine,
                          const uint8_t midstate[32], const uint8_t merkle_root_residue[4],
                          uint8_t sequence, uint8_t job_ctl) {
    if (!io || !midstate || !merkle_root_residue) return BZM_EINVAL;

    uint8_t buf[4 + 32 + 4 + 1 + 1];
    uint32_t h = bzm_header32(asic, BZM_OPCODE_WRITEJOB, engine, 0);
    write_u32_be(buf, h);
    memcpy(&buf[4], midstate, 32);
    memcpy(&buf[36], merkle_root_residue, 4); // raw bytes, no endianness transform
    buf[40] = sequence;
    buf[41] = job_ctl;
    return tx_payload(io, asic, buf, sizeof(buf));
}

bzm_status_t bzm_readresult(bzm_transport_t *io, uint8_t asic, bzm_result_t *out, uint32_t timeout_ms) {
    if (!io || !out) return BZM_EINVAL;
    uint8_t hdr[2];
    write_u16_be(hdr, bzm_header16(asic, BZM_OPCODE_READRESULT));
    bzm_status_t st = tx_payload(io, asic, hdr, sizeof(hdr));
    if (st != BZM_OK) return st;

    // Response: 8 bytes
    uint8_t resp[8];
    st = rx_exact(io, resp, sizeof(resp), timeout_ms);
    if (st != BZM_OK) return st;

    uint16_t eng_sts = ((uint16_t)resp[0] << 8) | resp[1];
    out->engine_id = (uint16_t)((eng_sts >> 4) & 0x0FFF);
    out->sts = (uint8_t)(eng_sts & 0x0F);
    memcpy(out->nonce, &resp[2], 4);
    out->sequence_id = resp[6];
    out->time = resp[7];
    return BZM_OK;
}

bzm_status_t bzm_dts_vs_read(bzm_transport_t *io, uint8_t asic, uint8_t *buf, size_t len, uint32_t timeout_ms) {
    if (!io || !buf || len == 0) return BZM_EINVAL;
    uint8_t hdr[2];
    write_u16_be(hdr, bzm_header16(asic, BZM_OPCODE_DTS_VS));
    bzm_status_t st = tx_payload(io, asic, hdr, sizeof(hdr));
    if (st != BZM_OK) return st;
    return rx_exact(io, buf, len, timeout_ms);
}

bzm_status_t bzm_loopback(bzm_transport_t *io, uint8_t asic, const uint8_t *data, size_t len,
                          uint8_t tar_or_pad, uint8_t *out, uint32_t timeout_ms) {
    if (!io || (!data && len) || !out) return BZM_EINVAL;
    if (len > 255) return BZM_EINVAL; // bound for safety

    uint8_t hdr[2];
    write_u16_be(hdr, bzm_header16(asic, BZM_OPCODE_LOOPBACK));

    // payload: count_minus_1, tar_or_pad, data[len]
    uint8_t stackbuf[2 + 1 + 1 + 255];
    memcpy(stackbuf, hdr, 2);
    stackbuf[2] = (uint8_t)((len ? len : 1) - 1); // if len==0, send 1 with no data
    stackbuf[3] = tar_or_pad;
    if (len) memcpy(&stackbuf[4], data, len);

    bzm_status_t st = tx_payload(io, asic, stackbuf, 2 + 2 + len);
    if (st != BZM_OK) return st;

    // Response: length + data echoed (no tar)
    uint8_t rlen;
    st = rx_exact(io, &rlen, 1, timeout_ms);
    if (st != BZM_OK) return st;
    size_t exp = (size_t)rlen + 1; // rlen is count_minus_1
    if (exp != (len ? len : 1)) return BZM_EIO;
    // read echoed data
    if (len) {
        st = rx_exact(io, out, len, timeout_ms);
        return st;
    } else {
        // No data expected; nothing more to read
        return BZM_OK;
    }
}

bzm_status_t bzm_noop(bzm_transport_t *io, uint8_t asic, uint32_t timeout_ms) {
    if (!io) return BZM_EINVAL;
    uint8_t hdr[2];
    write_u16_be(hdr, bzm_header16(asic, BZM_OPCODE_NOOP));
    bzm_status_t st = tx_payload(io, asic, hdr, sizeof(hdr));
    if (st != BZM_OK) return st;
    uint8_t resp[3];
    st = rx_exact(io, resp, sizeof(resp), timeout_ms);
    if (st != BZM_OK) return st;
    return (resp[0] == '2' && resp[1] == 'Z' && resp[2] == 'B') ? BZM_OK : BZM_EIO;
}
