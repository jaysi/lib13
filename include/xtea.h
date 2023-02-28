#ifndef XTEA_H
#define XTEA_H

#include "type13.h"
#include "pack13i.c"

#define MACRO(A) do { A; } while(0)
//#define CHARS2INT(ptr) unpacki32(*(uint32_t*)(ptr))
#define CHARS2INT(ptr) unpacki32((char*)(ptr))
#define INT2CHARS(ptr, val) MACRO( packi32(ptr, val) )
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*
  The recommended value for the "num_rounds" parameter is 32, not 64, 
  as each iteration of the loop does two Feistel-network rounds. 
  To additionally improve speed, the loop can be unrolled by pre-computing the 
  values of sum+key[].
*/
void XTEA_init_key(uint32_t * k, const char *key);
void XTEA_encipher_block(char *data, const uint32_t * k);
void XTEA_ctr_crypt(char *data, int size, const char *key);
void XTEA_cbcmac(char *mac, const char *data, int size, const char *key);
void XTEA_davies_meyer(char *out, const char *in, int ilen);
void XTEA_encipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]);
void XTEA_decipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]);

#else

#endif
