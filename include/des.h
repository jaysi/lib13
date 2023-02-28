#ifndef _DES_H
#define _DES_H

#include "type13.h"

#define DES_BSIZE	8
#define DES3_BSIZE	8

#define DES_KSIZE	8
#define DES3_KSIZE	8

typedef struct des_context_t{
    uint8_t esk[32];		/* DES encryption subkeys */
    uint8_t dsk[32];		/* DES decryption subkeys */
} des_context;

typedef struct des3_context_t{
    uint8_t esk[96];		/* Triple-DES encryption subkeys */
    uint8_t dsk[96];		/* Triple-DES decryption subkeys */
} des3_context;

int des_set_key(des_context * ctx, uint8_t key[DES_KSIZE]);
void des_encrypt(des_context * ctx, uint8_t input[DES_BSIZE], uint8_t output[DES_BSIZE]);
void des_decrypt(des_context * ctx, uint8_t input[DES_BSIZE], uint8_t output[DES_BSIZE]);

int des3_set_2keys(des3_context * ctx, uint8_t key1[DES3_KSIZE], uint8_t key2[DES3_KSIZE]);
int des3_set_3keys(des3_context * ctx, uint8_t key1[DES3_KSIZE], uint8_t key2[DES3_KSIZE],
           uint8_t key3[DES3_KSIZE]);

void des3_encrypt(des3_context * ctx, uint8_t input[DES3_BSIZE], uint8_t output[DES3_BSIZE]);
void des3_decrypt(des3_context * ctx, uint8_t input[DES3_BSIZE], uint8_t output[DES3_BSIZE]);

#endif				/* des.h */
