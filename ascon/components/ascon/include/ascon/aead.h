// Ascon-128 AEAD API wrapper for ascon-c library.
// This header provides a small, stable API used by the apps,
// implemented by components/ascon/ascon_wrapper.c calling ascon-c.

#pragma once

#include <stddef.h>
#include <stdint.h>

#define ASCON_AEAD_KEY_LEN 16
#define ASCON_AEAD_NONCE_LEN 16
#define ASCON_AEAD_TAG_LEN 16

int ascon_aead128_encrypt(uint8_t* c, size_t* clen,
                          const uint8_t* m, size_t mlen,
                          const uint8_t* ad, size_t adlen,
                          const uint8_t* npub, const uint8_t* k);

int ascon_aead128_decrypt(uint8_t* m, size_t* mlen,
                          const uint8_t* c, size_t clen,
                          const uint8_t* ad, size_t adlen,
                          const uint8_t* npub, const uint8_t* k);
