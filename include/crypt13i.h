#ifndef CRYPT13I_H
#define CRYPT13I_H

#include "crypt13.h"

#include "des.h"
#include "aes.h"
#include "rc4.h"
#include "xtea.h"
#include "base64.h"
#include "../../libntru/src/ntru.h"
#include "../../libntru/src/key.h"

#include "const13.h"

#define C13I_KEYSIZE (256)
#define C13I_AES_KEYSIZE AES_KEY256
#define C13I_DES3_KEYSIZE (256)//actually 2*8*8 or 2*8*8 bits, for sha256

#define C13I_NTOTAL 7 //helper to list function
#define C13I_NCIPHER 5
#define C13I_NBLOCK 4
#define C13I_NSTREAM (C13I_NTOTAL-C13I_NBLOCK)
#define C13I_NPUBKEY (C13I_NTOTAL-C13I_NCIPHER)

#define C13I_LONGEST_NAME 6 //ECIES, helper to list function

#define _crypt13_is_init(s) ((s)->magic==MAGIC13_CR13?1:0)
#define _crypt13_copys(dst, src) memcpy(dst, src, sizeof(struct crypt13))
#define _crypt13_cryptblocks(s, insize) (insize%(s)->blocksize?((insize/(s)->blocksize)+1):(insize/(s)->blocksize))

struct crypt13 crypt13_alg[] = {
    {MAGIC13_CR13, CRYPT13_ALG_DES, CRYPT13_MODE_BLOCK, CRYPT13_TYPE_CIPHER, DES_BSIZE, DES_KSIZE, 0, "DES", NULL, NULL, NULL},
    {MAGIC13_CR13, CRYPT13_ALG_DES3, CRYPT13_MODE_BLOCK, CRYPT13_TYPE_CIPHER, DES3_BSIZE, DES3_KSIZE, 0, "DES3", NULL, NULL, NULL},
    {MAGIC13_CR13, CRYPT13_ALG_AES, CRYPT13_MODE_BLOCK, CRYPT13_TYPE_CIPHER, AES_BSIZE, 0, 0, "AES", NULL, NULL, NULL},
    {MAGIC13_CR13, CRYPT13_ALG_RC4, CRYPT13_MODE_STREAM, CRYPT13_TYPE_CIPHER, 0, 0, 0, "RC4", NULL, NULL, NULL},
    {MAGIC13_CR13, CRYPT13_ALG_XTEA, CRYPT13_MODE_BLOCK, CRYPT13_TYPE_CIPHER, 0, 0, 0, "XTEA", NULL, NULL, NULL},
    {MAGIC13_CR13, CRYPT13_ALG_NTRU, CRYPT13_MODE_BLOCK, CRYPT13_TYPE_PUBKEY, 0, 0, CRYPT13_FLAG_HASPAYLOAD, "NTRU", NULL, NULL, NULL},
    {MAGIC13_CR13, CRYPT13_ALG_BASE64, CRYPT13_MODE_STREAM, CRYPT13_TYPE_CHANGE, 0, 0, 0, "BASE64", NULL, NULL, NULL},
    {MAGIC13_CR13, CRYPT13_ALG_NONE, CRYPT13_MODE_NONE, CRYPT13_TYPE_NONE, 1, 1, 0, "NULL", NULL, NULL, NULL}
};

#endif // CRYPT13I_H
