#include "crypto_aead/crypto_aead.h"
#include "ascon/aead.h"

int aead_encrypt(uint8_t* c, size_t* clen,
                 const uint8_t* m, size_t mlen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k)
{
    return ascon_aead128_encrypt(c, clen, m, mlen, ad, adlen, npub, k);
}

int aead_decrypt(uint8_t* m, size_t* mlen,
                 const uint8_t* c, size_t clen,
                 const uint8_t* ad, size_t adlen,
                 const uint8_t* npub, const uint8_t* k)
{
    return ascon_aead128_decrypt(m, mlen, c, clen, ad, adlen, npub, k);
}

