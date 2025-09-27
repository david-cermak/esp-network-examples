#include <stddef.h>
#include <stdint.h>

// Prototypes from ascon-c (avoid including test headers)
int crypto_aead_encrypt(unsigned char *c, unsigned long long *clen,
                        const unsigned char *m, unsigned long long mlen,
                        const unsigned char *ad, unsigned long long adlen,
                        const unsigned char *nsec, const unsigned char *npub,
                        const unsigned char *k);

int crypto_aead_decrypt(unsigned char *m, unsigned long long *mlen,
                        unsigned char *nsec, const unsigned char *c,
                        unsigned long long clen, const unsigned char *ad,
                        unsigned long long adlen, const unsigned char *npub,
                        const unsigned char *k);

#include "ascon/aead.h"

int ascon_aead128_encrypt(uint8_t* c, size_t* clen,
                          const uint8_t* m, size_t mlen,
                          const uint8_t* ad, size_t adlen,
                          const uint8_t* npub, const uint8_t* k)
{
    unsigned long long outlen = 0;
    int ret = crypto_aead_encrypt((unsigned char*)c, &outlen,
                                  (const unsigned char*)m, (unsigned long long)mlen,
                                  (const unsigned char*)ad, (unsigned long long)adlen,
                                  NULL,
                                  (const unsigned char*)npub,
                                  (const unsigned char*)k);
    if (clen) *clen = (size_t)outlen;
    return ret;
}

int ascon_aead128_decrypt(uint8_t* m, size_t* mlen,
                          const uint8_t* c, size_t clen,
                          const uint8_t* ad, size_t adlen,
                          const uint8_t* npub, const uint8_t* k)
{
    unsigned long long outlen = 0;
    int ret = crypto_aead_decrypt((unsigned char*)m, &outlen,
                                  NULL,
                                  (const unsigned char*)c, (unsigned long long)clen,
                                  (const unsigned char*)ad, (unsigned long long)adlen,
                                  (const unsigned char*)npub,
                                  (const unsigned char*)k);
    if (mlen) *mlen = (size_t)outlen;
    return ret;
}

