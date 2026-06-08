#include "ntt.h"
#include "polyvec.h"
#include "../common/prng.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

static inline int64_t mod_q(int64_t x)
{
    x %= RACCOON_Q;
    if (x < 0) x += RACCOON_Q;
    return x;
}

/* ------------------------------------------------------------------ */
/* polyvec_l                                                          */
/* ------------------------------------------------------------------ */

void polyvec_l_zero(polyvec_l_t *v)
{
    for (int i = 0; i < QS_L; i++) poly_zero(&v->vec[i]);
}

void polyvec_l_copy(polyvec_l_t *dst, const polyvec_l_t *src)
{
    for (int i = 0; i < QS_L; i++) poly_copy(&dst->vec[i], &src->vec[i]);
}

void polyvec_l_add(polyvec_l_t *r, const polyvec_l_t *a, const polyvec_l_t *b)
{
    for (int i = 0; i < QS_L; i++) poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}

void polyvec_l_sub(polyvec_l_t *r, const polyvec_l_t *a, const polyvec_l_t *b)
{
    for (int i = 0; i < QS_L; i++) poly_sub(&r->vec[i], &a->vec[i], &b->vec[i]);
}

void polyvec_l_mul_scalar(polyvec_l_t *v, int64_t scalar)
{
    for (int i = 0; i < QS_L; i++) {
        for (int j = 0; j < QS_N; j++) {
            /* 128-bit intermediate needed: scalar < q (~2^49), coeff < q */
            __int128 tmp = (__int128)v->vec[i].coeffs[j] * scalar;
            int64_t r = (int64_t)(tmp % RACCOON_Q);
            if (r < 0) r += RACCOON_Q;
            v->vec[i].coeffs[j] = r;
        }
    }
}

int polyvec_l_equal(const polyvec_l_t *a, const polyvec_l_t *b)
{
    for (int i = 0; i < QS_L; i++)
        for (int j = 0; j < QS_N; j++)
            if (a->vec[i].coeffs[j] != b->vec[i].coeffs[j]) return 0;
    return 1;
}

void polyvec_l_uniform(polyvec_l_t *v, qs_prng_t *prng)
{
    for (int i = 0; i < QS_L; i++) {
        for (int j = 0; j < QS_N; j++) {
            /* 7-byte rejection sampling for 49-bit q */
            uint8_t buf[7];
            uint64_t r;
            do {
                prng_squeeze(prng, buf, 7);
                r  = (uint64_t)buf[0]
                   | ((uint64_t)buf[1] << 8)
                   | ((uint64_t)buf[2] << 16)
                   | ((uint64_t)buf[3] << 24)
                   | ((uint64_t)buf[4] << 32)
                   | ((uint64_t)buf[5] << 40)
                   | ((uint64_t)buf[6] << 48);
                r &= 0x0001FFFFFFFFFFFFULL; /* 49-bit mask */
            } while (r >= (uint64_t)RACCOON_Q);
            v->vec[i].coeffs[j] = (int64_t)r;
        }
    }
}

/*
 * Sample ephemeral vector r ~ uniform in [-B_w, B_w] where B_w = 2^nu_w.
 * Raccoon uses B_w = 2^44.  We sample 45 bits and reject if >= 2*B_w+1.
 */
void polyvec_l_sample_raccoon(polyvec_l_t *v, qs_prng_t *prng)
{
    const int64_t BW = RACCOON_BW;  /* 2^44 */
    for (int i = 0; i < QS_L; i++) {
        for (int j = 0; j < QS_N; j++) {
            uint8_t buf[6];
            int64_t r;
            do {
                prng_squeeze(prng, buf, 6);
                uint64_t u = (uint64_t)buf[0]
                           | ((uint64_t)buf[1] << 8)
                           | ((uint64_t)buf[2] << 16)
                           | ((uint64_t)buf[3] << 24)
                           | ((uint64_t)buf[4] << 32)
                           | ((uint64_t)buf[5] << 40);
                u &= 0x1FFFFFFFFFFFFULL;  /* 45-bit mask */
                r = (int64_t)u - BW;      /* center at 0 */
            } while (r < -BW || r > BW);
            /* Reduce to [0, q-1] */
            if (r < 0) r += RACCOON_Q;
            v->vec[i].coeffs[j] = r;
        }
    }
}

/*
 * out[i] = c * v[i]  for all i in [L]
 * Both operands must be in coefficient domain on entry;
 * output is returned in coefficient domain.
 */
void polyvec_l_mul_poly(polyvec_l_t *out, const poly_t *c, const polyvec_l_t *v)
{
    poly_t c_ntt;
    poly_copy(&c_ntt, c);
    poly_ntt(&c_ntt);

    for (int i = 0; i < QS_L; i++) {
        poly_t v_ntt;
        poly_copy(&v_ntt, &v->vec[i]);
        poly_ntt(&v_ntt);
        poly_pointwise_mul(&out->vec[i], &c_ntt, &v_ntt);
        poly_invntt(&out->vec[i]);
    }
}

int64_t polyvec_l_norm_inf(const polyvec_l_t *v)
{
    int64_t max = 0;
    for (int i = 0; i < QS_L; i++) {
        int64_t n = poly_norm_inf(&v->vec[i]);
        if (n > max) max = n;
    }
    return max;
}

/* ------------------------------------------------------------------ */
/* polyvec_k                                                          */
/* ------------------------------------------------------------------ */

void polyvec_k_zero(polyvec_k_t *v)
{
    for (int i = 0; i < QS_K; i++) poly_zero(&v->vec[i]);
}

void polyvec_k_copy(polyvec_k_t *dst, const polyvec_k_t *src)
{
    for (int i = 0; i < QS_K; i++) poly_copy(&dst->vec[i], &src->vec[i]);
}

void polyvec_k_add(polyvec_k_t *r, const polyvec_k_t *a, const polyvec_k_t *b)
{
    for (int i = 0; i < QS_K; i++) poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}

void polyvec_k_sub(polyvec_k_t *r, const polyvec_k_t *a, const polyvec_k_t *b)
{
    for (int i = 0; i < QS_K; i++) poly_sub(&r->vec[i], &a->vec[i], &b->vec[i]);
}

int polyvec_k_equal(const polyvec_k_t *a, const polyvec_k_t *b)
{
    for (int i = 0; i < QS_K; i++)
        for (int j = 0; j < QS_N; j++)
            if (a->vec[i].coeffs[j] != b->vec[i].coeffs[j]) return 0;
    return 1;
}

void polyvec_k_uniform(polyvec_k_t *v, qs_prng_t *prng)
{
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_N; j++) {
            uint8_t buf[7];
            uint64_t r;
            do {
                prng_squeeze(prng, buf, 7);
                r  = (uint64_t)buf[0]
                   | ((uint64_t)buf[1] << 8)
                   | ((uint64_t)buf[2] << 16)
                   | ((uint64_t)buf[3] << 24)
                   | ((uint64_t)buf[4] << 32)
                   | ((uint64_t)buf[5] << 40)
                   | ((uint64_t)buf[6] << 48);
                r &= 0x0001FFFFFFFFFFFFULL;
            } while (r >= (uint64_t)RACCOON_Q);
            v->vec[i].coeffs[j] = (int64_t)r;
        }
    }
}

void polyvec_k_mul_poly(polyvec_k_t *out, const poly_t *c, const polyvec_k_t *v)
{
    poly_t c_ntt;
    poly_copy(&c_ntt, c);
    poly_ntt(&c_ntt);

    for (int i = 0; i < QS_K; i++) {
        poly_t v_ntt;
        poly_copy(&v_ntt, &v->vec[i]);
        poly_ntt(&v_ntt);
        poly_pointwise_mul(&out->vec[i], &c_ntt, &v_ntt);
        poly_invntt(&out->vec[i]);
    }
}

/*
 * Left-shift every coefficient by `shift` bits (multiply by 2^shift), mod q.
 * Used for 2^{nu_t} * (c * t'_pk).
 */
void polyvec_k_shift_left(polyvec_k_t *out, const polyvec_k_t *in, int shift)
{
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_N; j++) {
            __int128 x = (__int128)in->vec[i].coeffs[j] << shift;
            int64_t r = (int64_t)(x % RACCOON_Q);
            if (r < 0) r += RACCOON_Q;
            out->vec[i].coeffs[j] = r;
        }
    }
}

int64_t polyvec_k_norm_inf(const polyvec_k_t *v)
{
    int64_t max = 0;
    for (int i = 0; i < QS_K; i++) {
        int64_t n = poly_norm_inf(&v->vec[i]);
        if (n > max) max = n;
    }
    return max;
}

/*
 * Round every coefficient: out[i] = centered(in[i]) >> NU_W
 * Used to compute w = round_{nu_w}(w_sum).
 */
void polyvec_k_round_nuw(polyvec_k_t *out, const polyvec_k_t *in)
{
    for (int i = 0; i < QS_K; i++) {
        for (int j = 0; j < QS_N; j++) {
            int64_t x = in->vec[i].coeffs[j];
            /* Centered representative in (-q/2, q/2] */
            if (x > RACCOON_Q / 2) x -= RACCOON_Q;
            /* Arithmetic right-shift by nu_w */
            int64_t rounded = x >> NU_W;
            /* Back to [0, q-1] */
            if (rounded < 0) rounded += RACCOON_Q;
            out->vec[i].coeffs[j] = rounded;
        }
    }
}
