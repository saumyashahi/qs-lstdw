#include <string.h>
#include "prng.h"
#include "hash.h"

void prng_init(qs_prng_t *ctx, const uint8_t seed[32])
{
    memcpy(ctx->seed, seed, 32);
    ctx->counter = 0;
}

void prng_squeeze(qs_prng_t *ctx, uint8_t *out, size_t outlen)
{
    uint8_t input[32 + 8];

    memcpy(input, ctx->seed, 32);

    for (int i = 0; i < 8; i++)
        input[32 + i] = (ctx->counter >> (56 - 8*i)) & 0xFF;

    shake256(out, outlen, input, sizeof(input));

    ctx->counter++;
}

uint32_t prng_uint32(qs_prng_t *ctx)
{
    uint8_t buf[4];

    prng_squeeze(ctx, buf, 4);

    return ((uint32_t)buf[0]) |
           ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}
