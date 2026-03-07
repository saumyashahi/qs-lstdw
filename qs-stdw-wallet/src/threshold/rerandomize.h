#ifndef RERANDOMIZE_H
#define RERANDOMIZE_H

#include <stdint.h>
#include <stddef.h>
#include "share.h"

void rerandomize_shares(party_secret_t parties[],
                        int N,
                        int T,
                        const uint8_t chaincode[CHAINCODE_BYTES],
                        const uint8_t *session_id,
                        size_t session_len);

#endif