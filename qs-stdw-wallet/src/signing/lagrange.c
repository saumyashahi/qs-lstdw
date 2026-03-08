#include "sign.h"
#include "../config/params.h"

/*
 * Lagrange interpolation coefficient for Shamir secret reconstruction.
 *
 * Algorithm 4, line 19:
 *   λ_{act,k} = LagrangeCoeff(k, act)
 *             = Π_{j ∈ act, j ≠ k} [ j · (j - k)^{-1} ]  mod Q
 *
 * All arithmetic is mod Q using the extended Euclidean algorithm for modular
 * inverse. This is the correct implementation for lattice-based threshold
 * signatures where shares live in Z_q.
 *
 * signer_id  : 1-based Shamir x-coordinate of this signer (k)
 * active_set : 1-based IDs of all T active signers
 * t          : threshold (|active_set|)
 *
 * Returns: λ_{act,k} ∈ [0, Q-1]
 */

/* Extended Euclidean: returns t such that a*t ≡ 1 (mod Q), or 0 if not invertible */
static int64_t modinv_q(int64_t a)
{
    int64_t t = 0, newt = 1;
    int64_t r = (int64_t)Q, newr = ((a % Q) + Q) % Q;

    while (newr != 0) {
        int64_t q   = r / newr;
        int64_t tmp = newt;
        newt = t - q * newt;
        t    = tmp;
        tmp  = newr;
        newr = r - q * newr;
        r    = tmp;
    }

    if (r > 1) return 0; /* a and Q are not coprime — shouldn't happen for valid IDs */
    if (t < 0) t += (int64_t)Q;
    return t;
}

int32_t qs_lagrange_coeff_modq(
    int signer_id,
    const int *active_set,
    int t
)
{
    int64_t num = 1; /* numerator   = Π j           */
    int64_t den = 1; /* denominator = Π (j - signer) */

    for (int i = 0; i < t; i++) {
        int j = active_set[i];
        if (j == signer_id)
            continue;

        /* num *= j  (mod Q) */
        num = (num * (int64_t)j) % (int64_t)Q;
        if (num < 0) num += (int64_t)Q;

        /* den *= (j - signer_id)  (mod Q) */
        int64_t diff = ((int64_t)j - signer_id) % (int64_t)Q;
        if (diff < 0) diff += (int64_t)Q;
        den = (den * diff) % (int64_t)Q;
    }

    /* λ = num * den^{-1}  mod Q */
    int64_t den_inv = modinv_q(den);
    int64_t lambda  = (num * den_inv) % (int64_t)Q;
    if (lambda < 0) lambda += (int64_t)Q;

    return (int32_t)lambda;
}