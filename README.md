# bzm — Bonanza Mine protocol library

Portable C17 protocol library for the Bonanza Mine (bzm) protocol over a 9‑bit UART
transport. The library is platform‑agnostic and re‑usable on host Linux/macOS
and embedded platforms (e.g., ESP32, STM32) via a small transport abstraction
you implement.

- Platforms: Linux, macOS (tested via CMake). Embedded toolchains supported by
  compiling `include/` + `src/` directly into firmware and providing a transport.
- Not supported: Windows desktop builds (explicitly disabled in CMake for now).

## Design
- Abstract transport: `bzm_transport_t` provides two callbacks that you implement:
  - `tx(ctx, asic, payload, len)`: send a 9‑bit address‑marked `asic` byte (9th bit=1),
    then `len` payload bytes (9th bit=0).
  - `rx(ctx, buf, len, timeout_ms)`: read exactly `len` bytes or fail/timeout.
- Endianness: 16/32‑bit headers serialized big‑endian; payload bytes are copied
  verbatim. Works the same on ARM/x86.
- Timeouts: all receive operations accept `timeout_ms` from the caller (no fixed
  delays inside the library).
- No platform code: only C11 headers (`stdint.h`, `stddef.h`, `string.h`). No malloc.

## Implemented Commands
- `WRITEREG` (0x2): write 1–248 bytes to engine/offset.
- `READREG` (0x3): read 1/2/4 bytes; sends TAR=0x08; returns data bytes.
- `MULTICAST WRITE` (0x4): like WRITEREG with group id in engine field.
- `WRITEJOB` (0x0, compact): midstate[32] + merkle_root_residue[4] + sequence + job_ctl.
- `READRESULT` (0x1): parses engine_id|sts (endian‑fixed) + nonce[4] + seq + time.
- `DTS/VS READ` (0xD): raw payload read into user buffer (gen‑dependent size).
- `LOOPBACK` (0xE): echo of length + data.
- `NOOP` (0xF): expects ASCII "2ZB".

Header helpers are provided:
- 32‑bit: `bzm_header32(asic, opcode, engine, offset)` → `asic:8|opcode:4|engine:12|offset:8`
- 16‑bit: `bzm_header16(asic, opcode)` → `asic:8|opcode:4`

Command helpers serialize headers to big-endian on the wire; payload bytes are copied
without modification.

See `include/bzm/bzm.h` for full API.

## Embedded (ESP32/STM32) Usage
1) Implement `bzm_transport_t` using your HAL/SDK:
   - ESP32 (ESP‑IDF): configure UART for 9‑bit mode and send the address byte with
     the 9th bit set, then payload bytes with the 9th bit clear. Implement `rx` with
     a blocking read or deadline‑based loop honoring `timeout_ms`.
   - STM32 (HAL/LL): set word length (9‑bit), use parity/stop per design, send the
     address byte with mark bit, then payload. Implement `rx` similarly with timeout.
2) Add `include/` to your include paths and compile `src/bzm_commands.c` into your
   firmware target. No other sources or libs are required.
3) Call the helpers (e.g., `bzm_writereg`, `bzm_readreg`, `bzm_writejob`, etc.) with
   appropriate `timeout_ms` for your platform.

The library does not provide concrete UART drivers; it only builds command frames
and parses responses.

## Host Build & Tests (Linux/macOS)
- Build: `cmake -S . -B build && cmake --build build -j`
- Tests: `ctest --test-dir build --output-on-failure`
- Valgrind (optional): `cmake --build build --target memcheck` if `valgrind` is installed

Testing uses the Unity framework fetched at configure time via CMake FetchContent.
To disable fetching (e.g., offline builds), configure with `-DBZM_FETCH_UNITY=OFF`.

Windows desktop builds are disabled in `CMakeLists.txt` for now; embedded toolchains are
supported by compiling sources directly.
