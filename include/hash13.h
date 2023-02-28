#ifndef HASH13_H
#define HASH13_H

#include "error13.h"
#include "sha256.h"

typedef uint8_t h13_hash8_t;
typedef uint8_t h13_crc8_t;
typedef uint16_t h13_crc16_t;
typedef uint32_t h13_crc32_t;
typedef uint32_t h13_hash32_t;
typedef uint8_t h13_hash256_t[32];

#define HASH13_NA   "n/a"

/*      hashes, from 0 to 63    */
#define H13_HASH_START      0
#define H13_HASH32_JOAT     1
#define H13_HASH8_PEARSON   2
#define H13_HASH_END        63
#define H13_HASH32_DEF  H13_HASH32_JOAT
#define H13_HASH8_DEF  H13_HASH8_PEARSON

/*      crypto-hashes, from 64 to 127   */
#define H13_CHASH_START     64
#define H13_CHASH256_SHA    65
#define H13_CHASH_END       127
#define H13_CHASH256_DEF    H13_CHASH256_SHA

/*      crc, from 128 to 191   */
#define H13_CRC_START       128
#define H13_CRC8_1          129
#define H13_CRC8_LRC        130
#define H13_CRC16_FLETCHER  131
#define H13_CRC32_FLETCHER  132
#define H13_CRC32_1         133
#define H13_CRC_END         191
#define H13_CRC8_DEF        H13_CRC8_LRC
#define H13_CRC16_DEF       H13_CRC16_FLETCHER
#define H13_CRC32_DEF       H13_CRC32_1

/*      reserved, 192 to 255        */
#define H13_RESERVED_START  192
#define H13_RESERVED_END    255

#ifdef __cplusplus
    extern "C" {
#endif

//security hash
error13_t h13_md5_file(char* path, char* hash);

//hash
uint32_t h13_joat(uint8_t *key, size_t key_len);
uint8_t h13_pearson(uint8_t *key, size_t len);

//crc
uint8_t h13_getcrc8(uint8_t *source, size_t number);
uint8_t h13_calculateLRC(const uint8_t *buf, size_t n);
uint16_t h13_fletcher16( uint8_t const *data, size_t bytes );
uint32_t h13_fletcher32( uint16_t const *data, size_t words );
uint32_t h13_getcrc32(const void *key, size_t len, uint32_t hash);
uint16_t h13_ipv4_16( uint8_t const *data, size_t bytes );

#define h13_sha256(key, len, digest)  sha256(key, len, digest)
#define h13_crc8(key, len)    h13_calculateLRC(key, len)
#define h13_crc16(key, len)   h13_fletcher16(key, len)
#define h13_crc32(key, len)   h13_getcrc32(key, len, 0xdeafbaff)
#define h13_hash32(key, len)  h13_joat(key, len)

#ifdef __cplusplus
    }
#endif

#endif // HASH13_H
