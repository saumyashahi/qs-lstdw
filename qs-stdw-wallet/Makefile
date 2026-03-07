CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -MMD -MP
INCLUDES = -Iconfig -Isrc -Isrc/common -Isrc/lattice -Isrc/keygen -Isrc/threshold
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

# ================================
# Test files
# ================================

SRC_TEST_POLY = tests/test_poly.c
SRC_TEST_MATRIX = tests/test_matrix.c
SRC_TEST_MASTER = tests/test_master_keygen.c
SRC_TEST_SHAMIR = tests/test_shamir.c
SRC_TEST_RERANDOMIZE = tests/test_rerandomize.c

# ================================
# Object files (core modules only)
# ================================

OBJS_COMMON = $(SRC_COMMON:.c=.o)
OBJS_LATTICE = $(SRC_LATTICE:.c=.o)
OBJS_KEYGEN = $(SRC_KEYGEN:.c=.o)

OBJS_CORE = $(OBJS_COMMON) $(OBJS_LATTICE) $(OBJS_KEYGEN) $(SRC_THRESHOLD:.c=.o)

DEPS = $(OBJS_CORE:.o=.d)

# ================================
# Targets
# ================================

all: test_poly test_matrix test_master

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
	      $(DEPS) \
	      test_poly test_matrix test_master test_shamir

-include $(DEPS)

.PHONY: all clean