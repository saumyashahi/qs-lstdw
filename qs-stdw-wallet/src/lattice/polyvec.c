#include "ntt.h"
#include "polyvec.h"
#include "../common/prng.h"

void polyvec_l_uniform(polyvec_l_t *v, qs_prng_t *prng)
{
    for(int i = 0; i < QS_L; i++)
    {
        for(int j = 0; j < QS_N; j++)
        {
            uint32_t r = prng_uint32(prng);
            v->vec[i].coeffs[j] = r % Q;
        }
    }
}

/* set vector to zero */

void polyvec_l_zero(polyvec_l_t *v)
{
    for(int i = 0; i < QS_L; i++)
        poly_zero(&v->vec[i]);
}


/* copy vector */

void polyvec_l_copy(polyvec_l_t *dst,
                    const polyvec_l_t *src)
{
    for(int i = 0; i < QS_L; i++)
        poly_copy(&dst->vec[i], &src->vec[i]);
}


/* r = a + b */

void polyvec_l_add(polyvec_l_t *r,
                   const polyvec_l_t *a,
                   const polyvec_l_t *b)
{
    for(int i = 0; i < QS_L; i++)
        poly_add(&r->vec[i],
                 &a->vec[i],
                 &b->vec[i]);
}


/* r = a - b */

void polyvec_l_sub(polyvec_l_t *r,
                   const polyvec_l_t *a,
                   const polyvec_l_t *b)
{
    for(int i = 0; i < QS_L; i++)
        poly_sub(&r->vec[i],
                 &a->vec[i],
                 &b->vec[i]);
}


/* scalar multiply */

void polyvec_l_mul_scalar(polyvec_l_t *v,
                          int32_t scalar)
{
    for(int i = 0; i < QS_L; i++)
    {
        for(int j = 0; j < QS_N; j++)
        {
            int64_t tmp =
                (int64_t)v->vec[i].coeffs[j] * scalar;

            tmp %= Q;
            if(tmp < 0) tmp += Q;

            v->vec[i].coeffs[j] = (int32_t)tmp;
        }
    }
}


/* equality check */

int polyvec_l_equal(const polyvec_l_t *a,
                    const polyvec_l_t *b)
{
    for(int i = 0; i < QS_L; i++)
    {
        for(int j = 0; j < QS_N; j++)
        {
            if(a->vec[i].coeffs[j] !=
               b->vec[i].coeffs[j])
                return 0;
        }
    }

    return 1;
}

/* polyvec_k operations */

void polyvec_k_zero(polyvec_k_t *v)
{
    for(int i = 0; i < QS_K; i++)
        poly_zero(&v->vec[i]);
}

void polyvec_k_copy(polyvec_k_t *dst,
                    const polyvec_k_t *src)
{
    for(int i = 0; i < QS_K; i++)
        poly_copy(&dst->vec[i], &src->vec[i]);
}

void polyvec_k_add(polyvec_k_t *r,
                   const polyvec_k_t *a,
                   const polyvec_k_t *b)
{
    for(int i = 0; i < QS_K; i++)
        poly_add(&r->vec[i],
                 &a->vec[i],
                 &b->vec[i]);
}

int polyvec_k_equal(const polyvec_k_t *a,
                    const polyvec_k_t *b)
{
    for(int i = 0; i < QS_K; i++)
    {
        for(int j = 0; j < QS_N; j++)
        {
            if(a->vec[i].coeffs[j] !=
               b->vec[i].coeffs[j])
                return 0;
        }
    }

    return 1;
}

void polyvec_k_uniform(polyvec_k_t *v,
                       qs_prng_t *prng)
{
    for(int i = 0; i < QS_K; i++)
    {
        for(int j = 0; j < QS_N; j++)
        {
            uint32_t r = prng_uint32(prng);
            v->vec[i].coeffs[j] = r % Q;
        }
    }
}

/* ---------------------------------------------------------------------------
 * New operations required for Algorithm 4 (Signing) and Algorithm 5 (Verify)
 * --------------------------------------------------------------------------- */

/*
 * out[i] = c * v[i]  for all i in [L]
 * Polynomial ring multiplication of each component by challenge poly c.
 */
/*
 * out[i] = c * v[i]  for all i in [L]
 *
 * Bug fix: poly_pointwise_mul requires BOTH operands to be in NTT domain.
 * The original code only converted c to NTT but left v->vec[i] in
 * coefficient domain, producing garbage results.
 *
 * Fix: convert each v->vec[i] to NTT before the pointwise multiplication,
 * then apply inverse NTT to recover the product in coefficient domain.
 * c_ntt is computed once and reused across all L components for efficiency.
 */
void polyvec_l_mul_poly(polyvec_l_t *out,
                        const poly_t *c,
                        const polyvec_l_t *v)
{
    poly_t c_ntt;
    poly_copy(&c_ntt, c);
    poly_ntt(&c_ntt);

    for (int i = 0; i < QS_L; i++)
    {
        poly_t v_ntt;
        poly_copy(&v_ntt, &v->vec[i]);
        poly_ntt(&v_ntt);                              /* FIX: NTT the vector component */
        poly_pointwise_mul(&out->vec[i], &c_ntt, &v_ntt);
        poly_invntt(&out->vec[i]);
    }
}

void polyvec_k_mul_poly(polyvec_k_t *out,
                        const poly_t *c,
                        const polyvec_k_t *v)
{
    poly_t c_ntt;
    poly_copy(&c_ntt, c);
    poly_ntt(&c_ntt);

    for (int i = 0; i < QS_K; i++)
    {
        poly_t v_ntt;
        poly_copy(&v_ntt, &v->vec[i]);
        poly_ntt(&v_ntt);                              /* FIX: NTT the vector component */
        poly_pointwise_mul(&out->vec[i], &c_ntt, &v_ntt);
        poly_invntt(&out->vec[i]);
    }
}

/*
 * r = a - b  (coefficient-wise, mod Q)
 */
void polyvec_k_sub(polyvec_k_t *r,
                   const polyvec_k_t *a,
                   const polyvec_k_t *b)
{
    for (int i = 0; i < QS_K; i++)
        poly_sub(&r->vec[i], &a->vec[i], &b->vec[i]);
}

/*
 * Sample ephemeral vector: coefficients uniform in [-(GAMMA1-1), GAMMA1]
 * Uses rejection sampling from PRNG output.
 */
void polyvec_l_sample_gamma1(polyvec_l_t *v, qs_prng_t *prng)
{
    for (int i = 0; i < QS_L; i++)
    {
        for (int j = 0; j < QS_N; j++)
        {
            uint32_t r;
            /* Rejection sample to avoid bias */
            do {
                r = prng_uint32(prng);
                r &= 0x3FFFF; /* 18-bit mask — covers 2*GAMMA1 = 2^18 */
            } while (r >= (uint32_t)(2 * GAMMA1));

            /* Map to [-GAMMA1+1, GAMMA1] */
            v->vec[i].coeffs[j] = (int32_t)r - GAMMA1 + 1;
            /* Reduce mod Q into [0, Q-1] */
            int64_t x = v->vec[i].coeffs[j];
            x %= Q;
            if (x < 0) x += Q;
            v->vec[i].coeffs[j] = (int32_t)x;
        }
    }
}

/*
 * Left-shift every coefficient by `shift` bits (multiply by 2^shift), mod Q.
 * Used to compute 2^{ν_t} * (c * t'_pk).
 */
void polyvec_k_shift_left(polyvec_k_t *out,
                          const polyvec_k_t *in,
                          int shift)
{
    for (int i = 0; i < QS_K; i++)
    {
        for (int j = 0; j < QS_N; j++)
        {
            int64_t x = (int64_t)in->vec[i].coeffs[j] << shift;
            x %= Q;
            if (x < 0) x += Q;
            out->vec[i].coeffs[j] = (int32_t)x;
        }
    }
}

/*
 * Infinity norm of polyvec_k using centered representative in (-Q/2, Q/2].
 */
int32_t polyvec_k_norm_inf(const polyvec_k_t *v)
{
    int32_t max = 0;
    for (int i = 0; i < QS_K; i++)
    {
        int32_t n = poly_norm_inf(&v->vec[i]);
        if (n > max) max = n;
    }
    return max;
}

/*
 * Infinity norm of polyvec_l using centered representative in (-Q/2, Q/2].
 */
int32_t polyvec_l_norm_inf(const polyvec_l_t *v)
{
    int32_t max = 0;
    for (int i = 0; i < QS_L; i++)
    {
        int32_t n = poly_norm_inf(&v->vec[i]);
        if (n > max) max = n;
    }
    return max;
}

/*
 * Round every coefficient: out[i] = in[i] >> NU_W  (arithmetic right-shift)
 * This implements round_{ν_w} applied to the whole polyvec.
 */
void polyvec_k_round_nuw(polyvec_k_t *out, const polyvec_k_t *in)
{
    for (int i = 0; i < QS_K; i++)
    {
        for (int j = 0; j < QS_N; j++)
        {
            int32_t x = in->vec[i].coeffs[j];

            /* Step 1: centered representative in (-Q/2, Q/2] */
            if (x > (int32_t)(Q / 2)) x -= (int32_t)Q;

            /* Step 2: arithmetic right-shift (rounds toward -inf) */
            int32_t rounded = x >> NU_W;

            /*
             * Step 3: normalize to [0, Q-1].
             *
             * This is critical for challenge hash consistency.
             * poly_add/poly_sub always produce results in [0,Q-1],
             * so the Hc serialization of w_agg_rounded (signer) and
             * w' = h + y (verifier) must be in the same representation.
             *
             * Without this: -5 (signed) and Q-5=8380412 (from poly_add)
             * produce completely different byte sequences for the same
             * Z_q element, causing the challenge check to always fail.
             */
            if (rounded < 0) rounded += (int32_t)Q;

            out->vec[i].coeffs[j] = rounded;
        }
    }
}