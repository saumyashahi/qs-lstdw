CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -MMD -MP
INCLUDES = -Iconfig -Isrc -Isrc/common -Isrc/lattice
LIBS = -lcrypto

# Source files
SRC_COMMON = src/common/hash.c src/common/prng.c
SRC_LATTICE = src/lattice/poly.c \
              src/lattice/poly_from_seed.c \
              src/lattice/matrix.c \
              src/lattice/sample.c

SRC_TEST_POLY = tests/test_poly.c
SRC_TEST_MATRIX = tests/test_matrix.c

# Object files (excluding test mains)
OBJS_COMMON = $(SRC_COMMON:.c=.o)
OBJS_LATTICE = $(SRC_LATTICE:.c=.o)
OBJS_CORE = $(OBJS_COMMON) $(OBJS_LATTICE)

DEPS = $(OBJS_CORE:.o=.d)

# Targets
all: test_poly test_matrix

test_poly: $(OBJS_CORE) $(SRC_TEST_POLY:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

test_matrix: $(OBJS_CORE) $(SRC_TEST_MATRIX:.c=.o)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

# Compile rule
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS_CORE) \
	      $(SRC_TEST_POLY:.c=.o) \
	      $(SRC_TEST_MATRIX:.c=.o) \
	      $(DEPS) \
	      test_poly test_matrix

-include $(DEPS)

.PHONY: all clean