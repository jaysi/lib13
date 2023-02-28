#include <stdint.h>
#include "include/xtea.h"
 

void XTEA_init_key(uint32_t * k, const char *key)
{
    k[0] = CHARS2INT(key + 0);
    k[1] = CHARS2INT(key + 4);
    k[2] = CHARS2INT(key + 8);
    k[3] = CHARS2INT(key + 12);
}

                             /* the XTEA block cipher */
void XTEA_encipher_block(char *data, const uint32_t * k)
{
    uint32_t sum = 0, delta = 0x9e3779b9, y, z;
    int i;
    y = CHARS2INT(data);
    z = CHARS2INT(data + 4);
    for (i = 0; i < 32; i++) {
        y += ((z << 4 ^ z >> 5) + z) ^ (sum + k[sum & 3]);
        sum += delta;
        z += ((y << 4 ^ y >> 5) + y) ^ (sum + k[sum >> 11 & 3]);
    }
    INT2CHARS(data, y);
    INT2CHARS(data + 4, z);
}

                               /* encrypt in CTR mode */
void XTEA_ctr_crypt(char *data, int size, const char *key)
{
    uint32_t k[4], ctr = 0;
    int len, i;
    char buf[8];
    XTEA_init_key(k, key);
    while (size) {
        INT2CHARS(buf, 0);
        INT2CHARS(buf + 4, ctr++);
        XTEA_encipher_block(buf, k);
        len = MIN(8, size);
        for (i = 0; i < len; i++)
            *data++ ^= buf[i];
        size -= len;
    }
}

                             /* calculate the CBC MAC */
void XTEA_cbcmac(char *mac, const char *data, int size, const char *key)
{
    uint32_t k[4];
    int len, i;
    XTEA_init_key(k, key);
    INT2CHARS(mac, 0);
    INT2CHARS(mac + 4, size);
    XTEA_encipher_block(mac, k);
    while (size) {
        len = MIN(8, size);
        for (i = 0; i < len; i++)
            mac[i] ^= *data++;
        XTEA_encipher_block(mac, k);
        size -= len;
    }
}

                     /* modified(!) Davies-Meyer construction. */
void XTEA_davies_meyer(char *out, const char *in, int ilen)
{
    uint32_t k[4];
    char buf[8];
    int i;
    memset(out, 0, 8);
    while (ilen--) {
        XTEA_init_key(k, in);
        memcpy(buf, out, 8);
        XTEA_encipher_block(buf, k);
        for (i = 0; i < 8; i++)
            out[i] ^= buf[i];
        in += 16;
    }
}

/* take 64 bits of data in v[0] and v[1] and 128 bits of key[0] - key[3] */
 
void XTEA_encipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]) {
    unsigned int i;
    uint32_t v0=v[0], v1=v[1], sum=0, delta=0x9E3779B9;
    for (i=0; i < num_rounds; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    v[0]=v0; v[1]=v1;
}

void XTEA_decipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]) {
    unsigned int i;
    uint32_t v0=v[0], v1=v[1], delta=0x9E3779B9, sum=delta*num_rounds;
    for (i=0; i < num_rounds; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    v[0]=v0; v[1]=v1;
}
