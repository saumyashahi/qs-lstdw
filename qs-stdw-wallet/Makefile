CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -MMD -MP
INCLUDES = -Iconfig -Isrc -Isrc/common -Isrc/lattice -Isrc/keygen -Isrc/threshold -Isrc/signing -Isrc/verify -Isrc/derivation
LIBS = -lcrypto

# ================================
# Source files
# ================================

SRC_COMMON = src/common/hash.c src/common/prng.c src/common/randombytes.c

SRC_LATTICE = src/lattice/poly.c \
              src/lattice/poly_from_seed.c \
              src/lattice/matrix.c \
              src/lattice/sample.c \
              src/lattice/polyvec.c

SRC_KEYGEN = src/keygen/master_keygen.c

SRC_THRESHOLD = src/threshold/shamir.c \
                src/threshold/rerandomize.c

SRC_RANDPK = src/derivation/rand_pk.c

SRC_SIGN = src/signing/challenge_hash.c \
           src/signing/challenge_poly.c \
           src/signing/combine.c \
           src/signing/commit.c \
           src/signing/lagrange.c \
           src/signing/response.c \
           src/signing/rounding.c \
           src/verify/verify.c

# ================================
# Test files
# ================================

SRC_TEST_POLY        = tests/test_poly.c
SRC_TEST_MATRIX      = tests/test_matrix.c
SRC_TEST_MASTER      = tests/test_master_keygen.c
SRC_TEST_SHAMIR      = tests/test_shamir.c
SRC_TEST_RERANDOMIZE = tests/test_rerandomize.c
SRC_TEST_RANDPK      = tests/test_randpk.c
SRC_TEST_SIGN        = tests/test_sign.c
SRC_TEST_VERIFY      = tests/test_verify.c
SRC_BENCHMARK        = tests/benchmark.c

# ================================
# Wallet & Network Simulation files
# ================================

SRC_WALLET = src/wallet/wallet_init.c src/wallet/wallet_sign.c src/wallet/wallet_verify.c
OBJS_WALLET = $(SRC_WALLET:.c=.o)

SRC_SIM = src/net/local_sim.c

# ================================
# Object files (core modules only)
# ================================

OBJS_COMMON    = $(SRC_COMMON:.c=.o)
OBJS_LATTICE   = $(SRC_LATTICE:.c=.o)
OBJS_KEYGEN    = $(SRC_KEYGEN:.c=.o)
OBJS_RANDPK    = $(SRC_RANDPK:.c=.o)
OBJS_THRESHOLD = $(SRC_THRESHOLD:.c=.o)
OBJS_SIGN      = $(SRC_SIGN:.c=.o)

OBJS_CORE = $(OBJS_COMMON) $(OBJS_LATTICE) $(OBJS_KEYGEN) $(OBJS_RANDPK) $(OBJS_THRESHOLD) $(OBJS_SIGN) $(OBJS_WALLET)

DEPS = $(OBJS_CORE:.o=.d) $(SRC_SIM:.c=.d)

# ================================
# Targets
# ================================

all: test_poly test_matrix test_master test_shamir test_rerandomize test_randpk test_sign test_verify local_sim benchmark

test_poly: $(OBJS_CORE) $(SRC_TEST_POLY:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_matrix: $(OBJS_CORE) $(SRC_TEST_MATRIX:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_master: $(OBJS_CORE) $(SRC_TEST_MASTER:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_shamir: $(OBJS_CORE) $(SRC_TEST_SHAMIR:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_rerandomize: $(OBJS_CORE) $(SRC_TEST_RERANDOMIZE:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_randpk: $(OBJS_CORE) $(SRC_TEST_RANDPK:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_sign: $(OBJS_CORE) $(SRC_TEST_SIGN:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_verify: $(OBJS_CORE) $(SRC_TEST_VERIFY:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

local_sim: $(OBJS_CORE) $(SRC_SIM:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

benchmark: $(OBJS_CORE) $(SRC_BENCHMARK:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

# ================================
# Compile rule
# ================================

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS_CORE) \
	      $(SRC_TEST_POLY:.c=.o) \
	      $(SRC_TEST_MATRIX:.c=.o) \
	      $(SRC_TEST_MASTER:.c=.o) \
	      $(SRC_TEST_SHAMIR:.c=.o) \
	      $(SRC_TEST_RERANDOMIZE:.c=.o) \
	      $(SRC_TEST_RANDPK:.c=.o) \
	      $(SRC_TEST_SIGN:.c=.o) \
	      $(SRC_TEST_VERIFY:.c=.o) \
	      $(SRC_BENCHMARK:.c=.o) \
	      $(SRC_SIM:.c=.o) \
	      $(DEPS) \
	      test_poly test_matrix test_master test_shamir test_rerandomize test_randpk test_sign test_verify local_sim benchmark

-include $(DEPS)

.PHONY: all clean