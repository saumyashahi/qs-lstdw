#ifndef QS_STDW_BUILD_CONFIG_H
#define QS_STDW_BUILD_CONFIG_H

/*************************************************
 * Build Configuration Flags
 *************************************************/

/* ===============================
 * Build Mode
 * =============================== */

// Enable for verbose logging and asserts
#define DEBUG_MODE 1

// Enable extra checks (slow but safe)
#define SANITY_CHECKS 1

// Disable constant-time hardening (demo only)
#define DEMO_MODE 1

/* ===============================
 * Logging
 * =============================== */

#if DEBUG_MODE
  #define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_DEBUG(fmt, ...)
#endif

#define LOG_INFO(fmt, ...)  printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

/* ===============================
 * Assertions
 * =============================== */

#if SANITY_CHECKS
  #include <assert.h>
  #define QS_ASSERT(x) assert(x)
#else
  #define QS_ASSERT(x)
#endif

/* ===============================
 * Feature Toggles
 * =============================== */

// Use bounded sampling instead of true Gaussian
#define USE_BOUNDED_SAMPLING 1

// Disable NTT (schoolbook multiplication)
#define USE_NTT 0

// Simulate networking locally
#define LOCAL_SIMULATION 1

#endif /* QS_STDW_BUILD_CONFIG_H */