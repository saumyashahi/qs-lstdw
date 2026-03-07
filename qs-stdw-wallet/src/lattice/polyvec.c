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
