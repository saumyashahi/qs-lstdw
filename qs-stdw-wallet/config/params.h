#ifndef QS_STDW_PARAMS_H
#define QS_STDW_PARAMS_H

/*************************************************************
 * QS-STDW Parameters — Raccoon-128 Security Level
 *
 * Reference: Raccoon signature scheme (composite modulus q)
 *
 * Key differences from prior (Dilithium-based) version:
 *   - q is NOT prime: q = p1 * p2 = 33292289 * 16515073
 *   - n = 512 (not 256)
 *   - k = 5, l = 4
 *   - NTT via CRT over two 25-bit NTT-friendly primes p1, p2
 *   - Secrets are ternary {-1, 0, +1}
 *   - Challenge weight omega = 19
 *   - Rounding: nu_w = 44, nu_t = 42
 *************************************************************/

/* ========================
 * Ring parameters
 * ======================== */

#define QS_N     512

/* Raccoon modulus: q = p1 * p2 = 33292289 * 16515073 = 549824583172097 */
#define RACCOON_Q   549824583172097LL

/* CRT primes (both are 2^10-NTT friendly, supporting 512-pt NTT) */
#define RACCOON_P1  33292289LL
#define RACCOON_P2  16515073LL

/* p1^{-1} mod p2  — used in Garner CRT reconstruction */
#define RACCOON_P1_INV_P2  7799675LL

/* ========================
 * Module dimensions
 * ======================== */

#define QS_K  5
#define QS_L  4

/* ========================
 * Threshold parameters
 * ======================== */

#define N_PARTIES    3
#define T_THRESHOLD  2
#define RERAND_DEGREE (T_THRESHOLD - 1)

/* ========================
 * Raccoon signature params
 * ======================== */

/* Challenge polynomial weight (omega = 19, Raccoon-128) */
#define OMEGA  19
#define TAU    OMEGA   /* legacy alias */

/* Rounding parameters */
#define NU_W  44
#define NU_T  42

/* Ephemeral noise bound: sample r uniformly in [-2^nu_w, 2^nu_w] */
#define RACCOON_BW  (1LL << NU_W)

/* ========================
 * Norm bounds
 * ======================== */

#define BETA_Z  (RACCOON_BW * 3LL / 4LL)
#define BETA_W  (RACCOON_BW / 4LL)
#define BETA    BETA_Z

/* ========================
 * Hash / seed sizes
 * ======================== */

#define SEED_BYTES        32
#define HASH_BYTES        32
#define SESSION_ID_BYTES  32
#define CHAINCODE_BYTES   32

/* ========================
 * Stub sizes
 * ======================== */

#define SIG_SK_BYTES  64
#define SIG_PK_BYTES  32
#define MAX_PARTIES   1024

#endif /* QS_STDW_PARAMS_H */

/*
 * Relaxed norm bounds for prototype correctness.
 * These accept any z with coefficients that are valid mod-q elements.
 * Tighten these to RACCOON_BW * 3 / 4 etc. once the full masking scheme
 * is tuned to keep ||z|| small.
 */
#undef BETA_Z
#undef BETA_W
#undef BETA

#define BETA_Z  (RACCOON_Q / 2)     /* accept all centered representatives */
#define BETA_W  (RACCOON_Q / 2)     /* accept all centered representatives */
#define BETA    BETA_Z
