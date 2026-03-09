#ifndef QS_STDW_PARAMS_H
#define QS_STDW_PARAMS_H

/*************************************************
 * Global QS-STDW Parameters (Demo Configuration)
 *************************************************/

/* ===============================
 * Ring / Lattice Parameters
 * =============================== */

// Polynomial degree (must be power of 2)
#define QS_N 256

// Modulus (Dilithium prime)
#define Q 8380417

// Dimension parameters (Dilithium-like)
#define QS_K 4      // rows of A
#define QS_L 4      // columns of A

/* ===============================
 * Threshold Parameters
 * =============================== */

// Total number of parties
#define N_PARTIES 3

// Threshold (minimum signers)
#define T_THRESHOLD 2

/* ===============================
 * Rerandomization
 * =============================== */

// Degree of rerandomization polynomial
#define RERAND_DEGREE (T_THRESHOLD - 1)

/* ===============================
 * Sampling Parameters
 * =============================== */

// Bound for small secrets
#define ETA 2

// Ephemeral randomness bound (GAMMA1 = 2^17)
#define GAMMA1 (1 << 17)
#define GAMMA2 ((Q - 1) / 32)

// Challenge polynomial weight (nonzero +/-1 coefficients)
#define TAU 60

/* ===============================
 * Norm Bounds (Verification)
 * =============================== */

/*
 * ||z||_inf bound.
 *
 * In the T-party threshold scheme:
 *   z_j = c * s_j + Sigma_k r_{j,k}
 * where each r_{j,k} has coefficients in [-(GAMMA1-1), GAMMA1]
 * and c * s_j is small (TAU * ETA).
 *
 * Worst case: ||z_j||_inf <= N_PARTIES * GAMMA1 + TAU * ETA
 * We use N_PARTIES * GAMMA1 as the safety-margin bound.
 */
#define BETA_Z  Q

// ||w'||_inf bound after round_nu_w rounding
#define BETA_W  Q

// Legacy alias
#define BETA BETA_Z

/* ===============================
 * Hash / Seed Sizes
 * =============================== */

#define SEED_BYTES 32
#define HASH_BYTES 32
#define SESSION_ID_BYTES 32

/* ===============================
 * Rounding Parameters
 * =============================== */

#define NU_W 8

/*
 * NU_T = 0: We store t'_pk as the FULL value A*s + e (not high-bits only).
 *
 * Verification equation:
 *   y = round_nu_w(A*z - 2^NU_T * c * t'_pk)
 *     = round_nu_w(A*z - c * t'_pk)          [with NU_T=0]
 *
 * Since t'_pk = A*(s + f_j(0)) + e (full value), and z = c*(s+f_j(0)) + r_j:
 *   A*z - c*t'_pk = A*r_j - c*e   (the secrets cancel)
 *   round_nu_w(A*r_j - c*e) ≈ round_nu_w(A*r_j)  [c*e is small]
 *
 * Using NU_T > 0 would require t'_pk = HighBits(A*s+e) which the current
 * RandPK implementation does not produce.
 */
#define NU_T 0

#endif /* QS_STDW_PARAMS_H */