#ifndef QS_HASH_H
#define QS_HASH_H

#include <stddef.h>
#include <stdint.h>

/* Domain-separated hash functions */

void shake256(uint8_t *out, size_t outlen,
              const uint8_t *in, size_t inlen);

void H(uint8_t *out,
       const uint8_t *in, size_t inlen);

void Hc(uint8_t *out,
        const uint8_t *pk, size_t pklen,
        const uint8_t *m, size_t mlen,
        const uint8_t *w, size_t wlen);

void HSessionID(uint8_t *out,
                const uint8_t *m, size_t mlen,
                uint32_t ctr);

#endif