/*
 * $Id: rc4.h, the RC4 stream-cipher
 */

#ifndef RC4_H
#define RC4_H

#include "type13.h"

typedef struct rc4_ctx_t{
	uchar state[256];
	uchar x, y;
} rc4_ctx;

extern void rc4_init(uchar * key, uint32_t len, rc4_ctx * ctx);
extern void rc4(uchar * data, uint32_t len, rc4_ctx * ctx);

#endif
