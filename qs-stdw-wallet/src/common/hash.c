// Assumes Keccak’s shake256 function is available as:

// void shake256(uint8_t *out, size_t outlen,
//               const uint8_t *in, size_t inlen);

// If not, adapt to your external library.

#include <string.h>
#include <stdio.h>
#include "hash.h"
#include "../../config/params.h"
#include <openssl/evp.h>

void shake256(uint8_t *out, size_t outlen,
              const uint8_t *in, size_t inlen)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return;

    EVP_DigestInit_ex(ctx, EVP_shake256(), NULL);
    EVP_DigestUpdate(ctx, in, inlen);
    EVP_DigestFinalXOF(ctx, out, outlen);

    EVP_MD_CTX_free(ctx);
}

/* Domain separation prefixes */
#define DOMAIN_H         0x01
#define DOMAIN_HC        0x02
#define DOMAIN_SESSION   0x03

static void encode_u32(uint8_t out[4], uint32_t x) {
    out[0] = (x >> 24) & 0xFF;
    out[1] = (x >> 16) & 0xFF;
    out[2] = (x >> 8)  & 0xFF;
    out[3] = x & 0xFF;
}

void H(uint8_t *out,
       const uint8_t *in, size_t inlen)
{
    uint8_t buf[1 + inlen];
    buf[0] = DOMAIN_H;
    memcpy(buf + 1, in, inlen);

    shake256(out, HASH_BYTES, buf, sizeof(buf));
}

void Hc(uint8_t *out,
        const uint8_t *pk, size_t pklen,
        const uint8_t *m, size_t mlen,
        const uint8_t *w, size_t wlen)
{
    size_t total = 1 + pklen + mlen + wlen;
    uint8_t buf[total];

    buf[0] = DOMAIN_HC;
    memcpy(buf + 1, pk, pklen);
    memcpy(buf + 1 + pklen, m, mlen);
    memcpy(buf + 1 + pklen + mlen, w, wlen);

    shake256(out, HASH_BYTES, buf, total);
}

void HSessionID(uint8_t *out,
                const uint8_t *m, size_t mlen,
                uint32_t ctr)
{
    uint8_t ctr_bytes[4];
    encode_u32(ctr_bytes, ctr);

    size_t total = 1 + mlen + 4;
    uint8_t buf[total];

    buf[0] = DOMAIN_SESSION;
    memcpy(buf + 1, m, mlen);
    memcpy(buf + 1 + mlen, ctr_bytes, 4);

    shake256(out, SESSION_ID_BYTES, buf, total);
}