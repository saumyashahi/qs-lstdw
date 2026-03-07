#include "randombytes.h"
#include <stdio.h>
#include <stdlib.h>

void randombytes(uint8_t *out, size_t outlen)
{
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) {
        perror("Failed to open /dev/urandom");
        exit(EXIT_FAILURE);
    }

    size_t read_bytes = fread(out, 1, outlen, f);
    if (read_bytes != outlen) {
        perror("Failed to read random bytes");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fclose(f);
}