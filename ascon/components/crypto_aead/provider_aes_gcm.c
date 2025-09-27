#include "crypto_aead/crypto_aead.h"
#include <string.h>
#include "mbedtls/gcm.h"

_Static_assert(AEAD_KEY_LEN == 16, "AES-GCM provider requires 128-bit key");
_Static_assert(AEAD_NONCE_LEN == 12, "AES-GCM provider requires 96-bit nonce");
_Static_assert(AEAD_TAG_LEN == 16, "AES-GCM provider uses 128-bit tag");

int aead_encrypt(uint8_t* c, size_t* clen,
                 const uint8_t* m, size_t mlen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k)
{
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);
    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, k, AEAD_KEY_LEN * 8);
    if (ret != 0) { mbedtls_gcm_free(&ctx); return ret; }

    uint8_t* ct = c;
    uint8_t* tag = c + mlen;
    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT,
                                     mlen, npub, AEAD_NONCE_LEN,
                                     ad, adlen, m, ct, AEAD_TAG_LEN, tag);
    if (ret == 0 && clen) *clen = mlen + AEAD_TAG_LEN;
    mbedtls_gcm_free(&ctx);
    return ret;
}

int aead_decrypt(uint8_t* m, size_t* mlen,
                 const uint8_t* c, size_t clen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k)
{
    if (clen < AEAD_TAG_LEN) return -1;
    size_t ct_only = clen - AEAD_TAG_LEN;
    const uint8_t* ct = c;
    const uint8_t* tag = c + ct_only;
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);
    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, k, AEAD_KEY_LEN * 8);
    if (ret != 0) { mbedtls_gcm_free(&ctx); return ret; }
    ret = mbedtls_gcm_auth_decrypt(&ctx, ct_only, npub, AEAD_NONCE_LEN,
                                   ad, adlen, tag, AEAD_TAG_LEN, ct, m);
    if (ret == 0 && mlen) *mlen = ct_only;
    mbedtls_gcm_free(&ctx);
    return ret;
}

