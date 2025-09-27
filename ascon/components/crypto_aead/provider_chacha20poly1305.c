#include "crypto_aead/crypto_aead.h"
#include <string.h>
#include "mbedtls/chachapoly.h"

_Static_assert(AEAD_KEY_LEN == 32, "ChaCha20-Poly1305 requires 256-bit key");
_Static_assert(AEAD_NONCE_LEN == 12, "ChaCha20-Poly1305 requires 96-bit nonce");
_Static_assert(AEAD_TAG_LEN == 16, "ChaCha20-Poly1305 uses 128-bit tag");

int aead_encrypt(uint8_t* c, size_t* clen,
                 const uint8_t* m, size_t mlen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k)
{
    mbedtls_chachapoly_context ctx;
    mbedtls_chachapoly_init(&ctx);
    int ret = mbedtls_chachapoly_setkey(&ctx, k);
    if (ret != 0) { mbedtls_chachapoly_free(&ctx); return ret; }

    uint8_t* ct = c;
    uint8_t* tag = c + mlen;
    ret = mbedtls_chachapoly_encrypt_and_tag(&ctx, mlen, npub,
                                             ad, adlen, m, ct, tag);
    if (ret == 0 && clen) *clen = mlen + AEAD_TAG_LEN;
    mbedtls_chachapoly_free(&ctx);
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
    mbedtls_chachapoly_context ctx;
    mbedtls_chachapoly_init(&ctx);
    int ret = mbedtls_chachapoly_setkey(&ctx, k);
    if (ret != 0) { mbedtls_chachapoly_free(&ctx); return ret; }
    ret = mbedtls_chachapoly_auth_decrypt(&ctx, ct_only, npub, ad, adlen, tag, ct, m);
    if (ret == 0 && mlen) *mlen = ct_only;
    mbedtls_chachapoly_free(&ctx);
    return ret;
}

