#include "crypto_aead/crypto_aead.h"
#include <string.h>

// Insecure mock; for plumbing/perf comparisons only

static void fake_tag(uint8_t tag[AEAD_TAG_LEN], const uint8_t* c, size_t clen,
                     const uint8_t* ad, size_t adlen,
                     const uint8_t* npub, const uint8_t* k)
{
    uint8_t acc[AEAD_TAG_LEN] = {0};
    for (size_t i = 0; i < AEAD_TAG_LEN; ++i) acc[i] = k[i % AEAD_KEY_LEN] ^ npub[i % AEAD_NONCE_LEN];
    for (size_t i = 0; i < clen; ++i) acc[i % AEAD_TAG_LEN] ^= c[i];
    for (size_t i = 0; i < adlen; ++i) acc[i % AEAD_TAG_LEN] ^= ad[i];
    memcpy(tag, acc, AEAD_TAG_LEN);
}

int aead_encrypt(uint8_t* c, size_t* clen,
                 const uint8_t* m, size_t mlen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k)
{
    (void)ad; (void)adlen;
    for (size_t i = 0; i < mlen; ++i) {
        c[i] = m[i] ^ k[i % AEAD_KEY_LEN] ^ npub[i % AEAD_NONCE_LEN];
    }
    uint8_t tag[AEAD_TAG_LEN];
    fake_tag(tag, c, mlen, ad, adlen, npub, k);
    memcpy(c + mlen, tag, AEAD_TAG_LEN);
    if (clen) *clen = mlen + AEAD_TAG_LEN;
    return 0;
}

int aead_decrypt(uint8_t* m, size_t* mlen,
                 const uint8_t* c, size_t clen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k)
{
    if (clen < AEAD_TAG_LEN) return -1;
    size_t ct_only = clen - AEAD_TAG_LEN;
    const uint8_t* tag_in = c + ct_only;
    uint8_t tag[AEAD_TAG_LEN];
    fake_tag(tag, c, ct_only, ad, adlen, npub, k);
    if (memcmp(tag, tag_in, AEAD_TAG_LEN) != 0) return -1;
    for (size_t i = 0; i < ct_only; ++i) {
        m[i] = c[i] ^ k[i % AEAD_KEY_LEN] ^ npub[i % AEAD_NONCE_LEN];
    }
    if (mlen) *mlen = ct_only;
    return 0;
}

