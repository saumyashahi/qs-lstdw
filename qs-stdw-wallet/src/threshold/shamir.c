#include <stdint.h>
#include <string.h>
#include "../../config/params.h"
#include "../lattice/matrix.h"
#include "../lattice/poly.h"
#include "../common/prng.h"
#include "share.h"

/* Reduce to [0, q-1] */
static int64_t mod_q(int64_t x)
{
    x %= RACCOON_Q;
    if (x < 0) x += RACCOON_Q;
    return x;
}

/* x * y mod q using __int128 to avoid overflow (q is 49-bit) */
static int64_t mulmod_q(int64_t x, int64_t y)
{
    __int128 r = (__int128)x * y;
    int64_t  m = (int64_t)(r % RACCOON_Q);
    if (m < 0) m += RACCOON_Q;
    return m;
}

/* Extended Euclidean algorithm: a^{-1} mod q */
static int64_t modinv_q(int64_t a)
{
    int64_t t = 0, newt = 1;
    int64_t r = RACCOON_Q, newr = ((a % RACCOON_Q) + RACCOON_Q) % RACCOON_Q;
    while (newr) {
        int64_t quo = r / newr, tmp;
        tmp = newt; newt = t - quo * newt; t = tmp;
        tmp = newr; newr = r - quo * newr; r = tmp;
    }
    if (r > 1) return 0;
    if (t < 0) t += RACCOON_Q;
    return t;
}

void shamir_split(const polyvec_l_t *secret,
                  shamir_share_t shares[],
                  int n,
                  int t,
                  qs_prng_t *prng)
{
    int64_t coeffs[t];

    for (int i = 0; i < n; i++) {
        shares[i].id = i + 1;
        memset(&shares[i].value, 0, sizeof(polyvec_l_t));
    }

    for (int vec = 0; vec < QS_L; vec++) {
        for (int k = 0; k < QS_N; k++) {

            coeffs[0] = secret->vec[vec].coeffs[k];

            for (int j = 1; j < t; j++) {
                /* Sample random coefficient mod q using 7-byte rejection */
                uint8_t buf[7];
                uint64_t rv;
                do {
                    prng_squeeze(prng, buf, 7);
                    rv  = (uint64_t)buf[0]
                        | ((uint64_t)buf[1] <<  8)
                        | ((uint64_t)buf[2] << 16)
                        | ((uint64_t)buf[3] << 24)
                        | ((uint64_t)buf[4] << 32)
                        | ((uint64_t)buf[5] << 40)
                        | ((uint64_t)buf[6] << 48);
                    rv &= 0x0001FFFFFFFFFFFFULL;
                } while ((int64_t)rv >= RACCOON_Q);
                coeffs[j] = (int64_t)rv;
            }

            for (int s = 0; s < n; s++) {
                int64_t x = shares[s].id;
                int64_t y = 0;
                int64_t x_pow = 1;

                for (int j = 0; j < t; j++) {
                    y     = mod_q(y + mulmod_q(coeffs[j], x_pow));
                    x_pow = mulmod_q(x_pow, x);
                }
                shares[s].value.vec[vec].coeffs[k] = y;
            }
        }
    }
}

void shamir_reconstruct(polyvec_l_t *result,
                        const shamir_share_t shares[],
                        int t)
{
    memset(result, 0, sizeof(polyvec_l_t));

    for (int vec = 0; vec < QS_L; vec++) {
        for (int k = 0; k < QS_N; k++) {
            int64_t secret = 0;

            for (int j = 0; j < t; j++) {
                int64_t xj = shares[j].id;
                int64_t yj = shares[j].value.vec[vec].coeffs[k];

                int64_t num = 1, den = 1;

                for (int m = 0; m < t; m++) {
                    if (m == j) continue;
                    int64_t xm = shares[m].id;
                    /* num *= (-xm) mod q */
                    num = mulmod_q(num, mod_q(RACCOON_Q - xm));
                    /* den *= (xj - xm) mod q */
                    den = mulmod_q(den, mod_q(xj - xm));
                }

                int64_t inv     = modinv_q(den);
                int64_t lagrange = mulmod_q(num, inv);
                secret = mod_q(secret + mulmod_q(yj, lagrange));
            }
            result->vec[vec].coeffs[k] = secret;
        }
    }
}
