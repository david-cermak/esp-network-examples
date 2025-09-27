Purpose of the Demo

Explore Ascon lightweight cryptography on ESP32 in a non-TLS context.

Demonstrate authenticated encryption (AEAD) of small messages between two ESP32 tasks or devices, using Ascon instead of a full TLS stack.

Provide a reference point for performance, code size, memory use, and integration feasibility of Ascon on constrained devices.

Show how lightweight crypto can protect small IoT payloads (telemetry, sensor readings, control commands) where TLS may be too heavy.

Scope

Not a full TLS or standards-compliant secure channel.

Symmetric-key AEAD only (no key exchange, no certificate handling).

Simple transport (UDP or raw TCP sockets).

Minimal error handling and replay protection (sequence counter as AAD).

Meant for evaluation and learning, not production deployment.


Requirements
Crypto Layer

Use the ascon-c
 reference/optimized library.

Include Ascon-128 AEAD for encryption/decryption.

Optional: include Ascon-Hash/XOF for integrity checks or device fingerprinting.

Must enforce nonce uniqueness per key (e.g., counter or random with safeguard).

Project Setup

Target: ESP32 (ESP-IDF project).

Add ascon-c as a component/submodule.

Build with either the generic C or asm_esp32 backend for performance.

Provide CMake / component.mk integration for ESP-IDF.

Sender Task

Take plaintext payload (e.g., “Hello from ESP32”).

Construct nonce.

Encrypt with Ascon-AEAD (key, nonce, optional AAD).

Send (nonce || ciphertext || tag) over UDP/TCP.

Receiver Task

Receive packet.

Parse nonce, ciphertext, tag.

Decrypt and verify tag.

On success, output plaintext; on failure, discard.

Support Code

Simple random or counter-based nonce generator.

Minimal network wrapper for sending/receiving packets.

Logging (UART/serial output) for debugging.

Optional: performance timers (cycle count, latency).

Deliverables

ESP-IDF Project Skeleton

main.c with sender/receiver demo.

Integrated Ascon component.

Configurable key, nonce source.

README

Build & flash instructions.

Key points on nonce management and limitations.

Performance notes (code size, cycles, memory).

Next Steps

Implement sender/receiver skeleton.

Run performance tests on ESP32 (measure encryption/decryption throughput).

Evaluate code size and RAM use.

Optionally extend to:

Replay protection (sequence numbers).

Hash/XOF demo (firmware checksum).

Integration with COSE/OSCORE lightweight secure messaging.