#include "ntt.h"
#include "../../config/params.h"
#include "polyvec.h"

/*
 * NTT implementation for Q = 8380417, n = 256.
 *
 * Uses the Cooley-Tukey butterfly in Dilithium-style negacyclic NTT:
 *   - Primitive 512th root of unity: ROOT = 1753
 *   - Zeta table: zetas[k] = ROOT^{brv8(k)} mod Q  for k = 0..255
 *     where brv8 is the bit-reversal of an 8-bit index.
 *   - This matches the CRYSTALS-Dilithium reference implementation.
 *
 * Bug fixes from original:
 *   1. Zeta table was wrong (values not derived from ROOT=1753 for Q=8380417).
 *      Corrected: zetas[k] = 1753^{bitrev8(k)} mod 8380417.
 *   2. N_INV was 41978 (incorrect). Correct value: 256^{-1} mod Q = 8347681.
 *   3. Zeta index k in poly_ntt started at 0 (zetas[0]=1 is trivial and
 *      is not used in the standard Dilithium layout); fixed to start at k=1.
 */

#define NTT_N  256
#define NTT_Q  8380417

/*
 * Precomputed zeta table: zetas[k] = 1753^{bitrev8(k)} mod 8380417
 *
 * Generation:
 *   ROOT = 1753  (primitive 512th root of unity mod Q)
 *   bitrev8(k) = bit-reversal of 8-bit integer k
 *   zetas[k]   = pow(ROOT, bitrev8(k), Q)
 *
 * The forward NTT uses zetas[1..255] (skipping zetas[0]=1).
 * The inverse NTT traverses the same table in reverse.
 */
static const int32_t zetas[NTT_N] = {
       1, 4808194, 3765607, 3761513, 5178923, 5496691, 5234739, 5178987,
 7778734, 3542485, 2682288, 2129892, 3764867, 7375178,  557458, 7159240,
 5010068, 4317364, 2663378, 6705802, 4855975, 7946292,  676590, 7044481,
 5152541, 1714295, 2453983, 1460718, 7737789, 4795319, 2815639, 2283733,
 3602218, 3182878, 2740543, 4793971, 5269599, 2101410, 3704823, 1159875,
  394148,  928749, 1095468, 4874037, 2071829, 4361428, 3241972, 2156050,
 3415069, 1759347, 7562881, 4805951, 3756790, 6444618, 6663429, 4430364,
 5483103, 3192354,  556856, 3870317, 2917338, 1853806, 3345963, 1858416,
 3073009, 1277625, 5744944, 3852015, 4183372, 5157610, 5258977, 8106357,
 2508980, 2028118, 1937570, 4564692, 2811291, 5396636, 7270901, 4158088,
 1528066,  482649, 1148858, 5418153, 7814814,  169688, 2462444, 5046034,
 4213992, 4892034, 1987814, 5183169, 1736313,  235407, 5130263, 3258457,
 5801164, 1787943, 5989328, 6125690, 3482206, 4197502, 7080401, 6018354,
 7062739, 2461387, 3035980,  621164, 3901472, 7153756, 2925816, 3374250,
 1356448, 5604662, 2683270, 5601629, 4912752, 2312838, 7727142, 7921254,
  348812, 8052569, 1011223, 6026202, 4561790, 6458164, 6143691, 1744507,
    1753, 6444997, 5720892, 6924527, 2660408, 6600190, 8321269, 2772600,
 1182243,   87208,  636927, 4415111, 4423672, 6084020, 5095502, 4663471,
 8352605,  822541, 1009365, 5926272, 6400920, 1596822, 4423473, 4620952,
 6695264, 4969849, 2678278, 4611469, 4829411,  635956, 8129971, 5925040,
 4234153, 6607829, 2192938, 6653329, 2387513, 4768667, 8111961, 5199961,
 3747250, 2296099, 1239911, 4541938, 3195676, 2642980, 1254190, 8368000,
 2998219,  141835, 8291116, 2513018, 7025525,  613238, 7070156, 6161950,
 7921677, 6458423, 4040196, 4908348, 2039144, 6500539, 7561656, 6201452,
 6757063, 2105286, 6006015, 6346610,  586241, 7200804,  527981, 5637006,
 6903432, 1994046, 2491325, 6987258,  507927, 7192532, 7655613, 6545891,
 5346675, 8041997, 2647994, 3009748, 5767564, 4148469,  749577, 4357667,
 3980599, 2569011, 6764887, 1723229, 1665318, 2028038, 1163598, 5011144,
 3994671, 8368538, 7009900, 3020393, 3363542,  214880,  545376, 7609976,
 3105558, 7277073,  508145, 7826699,  860144, 3430436,  140244, 6866265,
 6195333, 3123762, 2358373, 6187330, 5365997, 6663603, 2926054, 7987710,
 8077412, 3531229, 4405932, 4606686, 1900052, 7598542, 1054478, 7648983,
};

static inline int32_t mod_q(int64_t a)
{
    a %= NTT_Q;
    if (a < 0) a += NTT_Q;
    return (int32_t)a;
}

/*
 * Forward NTT (Cooley-Tukey, in-place).
 *
 * Input:  coefficient-domain polynomial a in R_q = Z_q[x]/(x^256 + 1)
 * Output: NTT-domain representation a_hat
 *
 * Butterfly:
 *   t          = zeta * a[j+len]  mod q
 *   a[j+len]   = a[j] - t         mod q
 *   a[j]       = a[j] + t         mod q
 *
 * Zeta index k starts at 1 (zetas[0]=1 is not used).
 */
void poly_ntt(poly_t *a)
{
    int k = 1;

    for (int len = 128; len > 0; len >>= 1)
    {
        for (int start = 0; start < NTT_N; start += 2 * len)
        {
            int32_t zeta = zetas[k++];

            for (int j = start; j < start + len; j++)
            {
                int32_t t          = mod_q((int64_t)zeta * a->coeffs[j + len]);
                a->coeffs[j + len] = mod_q(a->coeffs[j] - t);
                a->coeffs[j]       = mod_q(a->coeffs[j] + t);
            }
        }
    }
}

/*
 * Inverse NTT (Gentleman-Sande, in-place).
 *
 * Input:  NTT-domain polynomial a_hat
 * Output: coefficient-domain polynomial a
 *
 * Butterfly:
 *   t          = a[j]
 *   a[j]       = t + a[j+len]          mod q
 *   a[j+len]   = zeta * (t - a[j+len]) mod q
 *
 * Zeta index k traverses the table from 255 down to 1 (reverse of forward NTT).
 * Final scaling by N^{-1} = 8347681 (= 256^{-1} mod 8380417).
 */
void poly_invntt(poly_t *a)
{
    int k = NTT_N - 1;   /* start at 255, end at 1 */

    for (int len = 1; len < NTT_N; len <<= 1)
    {
        for (int start = 0; start < NTT_N; start += 2 * len)
        {
            int32_t zeta = zetas[k--];

            for (int j = start; j < start + len; j++)
            {
                int32_t t          = a->coeffs[j];
                a->coeffs[j]       = mod_q(t + a->coeffs[j + len]);
                a->coeffs[j + len] = mod_q((int64_t)zeta * (t - a->coeffs[j + len]));
            }
        }
    }

    /* Scale by N^{-1} = 256^{-1} mod 8380417 = 8347681 */
    for (int i = 0; i < NTT_N; i++)
        a->coeffs[i] = mod_q((int64_t)a->coeffs[i] * 8347681);
}

/*
 * Pointwise multiplication in NTT domain.
 * PRECONDITION: both a and b must be in NTT domain (output of poly_ntt).
 */
void poly_pointwise_mul(poly_t *r,
                        const poly_t *a,
                        const poly_t *b)
{
    for (int i = 0; i < NTT_N; i++)
        r->coeffs[i] = mod_q((int64_t)a->coeffs[i] * b->coeffs[i]);
}

/* ----- Vector wrappers ----- */

void polyvec_l_ntt(polyvec_l_t *v)
{
    for (int i = 0; i < QS_L; i++)
        poly_ntt(&v->vec[i]);
}

void polyvec_l_invntt(polyvec_l_t *v)
{
    for (int i = 0; i < QS_L; i++)
        poly_invntt(&v->vec[i]);
}

void polyvec_k_ntt(polyvec_k_t *v)
{
    for (int i = 0; i < QS_K; i++)
        poly_ntt(&v->vec[i]);
}

void polyvec_k_invntt(polyvec_k_t *v)
{
    for (int i = 0; i < QS_K; i++)
        poly_invntt(&v->vec[i]);
}
