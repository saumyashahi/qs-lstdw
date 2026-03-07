#ifndef QS_PRNG_H
#define QS_PRNG_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t seed[32];
    uint64_t counter;
} qs_prng_t;

void prng_init(qs_prng_t *ctx, const uint8_t seed[32]);
void prng_squeeze(qs_prng_t *ctx, uint8_t *out, size_t outlen);

uint32_t prng_uint32(qs_prng_t *ctx);

#endif