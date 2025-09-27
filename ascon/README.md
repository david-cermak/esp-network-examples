ASCON AEAD on ESP32 (UDP Demo)

Overview

- Demonstrates lightweight authenticated encryption (AEAD) on ESP32 without TLS.
- Sends small payloads over UDP between two ESP‑IDF apps: `sender/` and `receiver/`.
- Focuses on integration feasibility, nonce handling, and on‑device performance.

Status

- AEAD providers: Ascon‑128 (ascon‑c), AES‑GCM‑128, ChaCha20‑Poly1305 (mbedTLS) implemented and selectable via Kconfig.
- Transport: UDP working end‑to‑end with periodic messages from sender to receiver.
- Nonce/AAD: 128‑bit nonce with a monotonically increasing 64‑bit counter; the counter is also used as AAD to bind sequence.
- Logging: Hex dumps and microsecond timings for encrypt/decrypt paths.
- Not included yet: replay window, key exchange, tests/vectors integration.

Layout

- `components/ascon/` — ascon‑c integration and thin wrapper API.
- `components/crypto_aead/` — provider abstraction + Kconfig to select AEAD.
- `sender/` — UDP sender app; prints `enc_us` timings.
- `receiver/` — UDP receiver app; prints `dec_us` timings and plaintext on success.

Build and Run

Prereqs: ESP‑IDF v5+, an ESP32 board, and Wi‑Fi credentials.

Sender

1) `cd sender`
2) `idf.py set-target esp32`
3) `idf.py menuconfig`
   - Example Connection Configuration: set Wi‑Fi SSID/PASS.
   - Crypto AEAD: choose algorithm (default can be overridden by `sdkconfig.defaults`).
4) `idf.py build flash monitor`

Receiver

1) `cd receiver`
2) `idf.py set-target esp32`
3) `idf.py menuconfig`
   - Example Connection Configuration: set Wi‑Fi SSID/PASS.
   - Crypto AEAD: choose the same algorithm as the sender.
4) `idf.py build flash monitor`

Configuration Notes

- Algorithm selection: Menu “Crypto AEAD” exposes Ascon‑128, AES‑GCM‑128, and ChaCha20‑Poly1305.
- Key/nonce/tag sizes are exposed to apps via `AEAD_KEY_LEN`, `AEAD_NONCE_LEN`, `AEAD_TAG_LEN` in `crypto_aead.h`.
- Receiver address/port: sender uses `RECEIVER_IP` and `RECEIVER_PORT` defaults (192.168.0.35:3333). Adjust in `sender/main/sender.c` or define `CONFIG_ASCON_RECEIVER_IP` and `CONFIG_ASCON_RECEIVER_PORT` via build flags.
- Listen port: receiver uses `LISTEN_PORT` default 3333. Adjust in `receiver/main/receiver.c` or define `CONFIG_ASCON_LISTEN_PORT` via build flags.
- Nonce management: 64‑bit big‑endian counter in the high bytes of a 128‑bit nonce. Ensure uniqueness per key in any adaptation.

Performance

- The table below shows measured encrypt/decrypt timing for a short payload on ESP32; use it as a relative comparison across providers.

| AEAD | enc/us | dec/us |
|------|--------|--------|
| Ascon-128a | 195 +/- 2 | 183 +/- 2 |
| AES-GCM-128 | 595 +/- 18 | 560 +/- 15 |
| ChaCha20-Poly1305 | 530 +/- 12 | 510 +/- 16 |

Scope and Caveats

- Symmetric‑key AEAD only; no key exchange or certificates.
- Minimal error handling; no replay protection beyond binding a counter in AAD.
- Intended for evaluation/learning, not production.

Next Steps

- Add replay window and monotonic counter persistence.
- Add Unity tests and known‑good test vectors for ascon and providers.
- Optional: include Ascon‑Hash/XOF example (e.g., firmware checksum/fingerprint).
- Explore COSE/OSCORE style lightweight secure messaging on top of AEAD.

Notes for Contributors

- Coding: C (4‑space indent), headers use `#pragma once`, avoid VLAs.
- Formatting: run `tools/format.sh` if present; keep files UTF‑8 with LF.
- Commits: Conventional Commits style (`feat:`, `fix:`, `perf:`, `test:`, `docs:`).
