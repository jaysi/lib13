#ifndef _AES_H
#define _AES_H

#include "type13.h"

#define AES_BSIZE	16
#define AES_KEY128  128
#define AES_KEY192  192
#define AES_KEY256  256

typedef struct aes_context_t{
    uint32_t erk[64];		/* encryption round keys */
    uint32_t drk[64];		/* decryption round keys */
    int nr;             /* number of rounds */
} aes_context;

#ifdef __cplusplus
    extern "C" {
#endif

int aes_set_key(    aes_context * ctx, uint8_t * key, int nbits);
void aes_encrypt(   aes_context * ctx, uint8_t input[AES_BSIZE],
                    uint8_t output[AES_BSIZE]);
void aes_decrypt(   aes_context * ctx, uint8_t input[AES_BSIZE],
                    uint8_t output[AES_BSIZE]);

#ifdef __cplusplus
    }
#endif

#endif  /* aes.h */
