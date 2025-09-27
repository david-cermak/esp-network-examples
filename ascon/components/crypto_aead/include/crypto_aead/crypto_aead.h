#pragma once

#include <stddef.h>
#include <stdint.h>
#include "sdkconfig.h"

// Algorithm-dependent sizes
#if CONFIG_AEAD_IMPL_ASCON
  #define AEAD_KEY_LEN   16
  #define AEAD_NONCE_LEN 16
  #define AEAD_TAG_LEN   16
#elif CONFIG_AEAD_IMPL_AES_GCM
  #define AEAD_KEY_LEN   16
  #define AEAD_NONCE_LEN 12
  #define AEAD_TAG_LEN   16
#elif CONFIG_AEAD_IMPL_CHACHA20_POLY1305
  #define AEAD_KEY_LEN   32
  #define AEAD_NONCE_LEN 12
  #define AEAD_TAG_LEN   16
#elif CONFIG_AEAD_IMPL_MOCK
  #define AEAD_KEY_LEN   16
  #define AEAD_NONCE_LEN 16
  #define AEAD_TAG_LEN   16
#else
  #error "No AEAD implementation selected"
#endif

int aead_encrypt(uint8_t* c, size_t* clen,
                 const uint8_t* m, size_t mlen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k);

int aead_decrypt(uint8_t* m, size_t* mlen,
                 const uint8_t* c, size_t clen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k);

