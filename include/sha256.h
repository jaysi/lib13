#ifndef _SHA256_H
#define _SHA256_H

#include "type13.h"

#define SHA256_HASH_LEN 32

typedef struct sha256_context_t{
    uint32_t total[2];
    uint32_t state[8];
    uint8_t buffer[64];
} sha256_context;

void sha256_starts(sha256_context * ctx);
void sha256_update(sha256_context * ctx, uint8_t * input, uint32_t length);
void sha256_finish(sha256_context * ctx, uint8_t digest[SHA256_HASH_LEN]);
void sha256(uint8_t * input, uint32_t len, uint8_t digest[SHA256_HASH_LEN]);

#endif				/* sha256.h */
