#ifndef CRYPT13_H
#define CRYPT13_H

#include "type13.h"
#include "error13.h"

typedef uint8_t crypt13_alg_t;
typedef uint8_t crypt13_mode_t;
typedef uint8_t crypt13_type_t;
typedef uint8_t crypt13_flag_t;

// UPDATE crypt13_alg_id() on CHANGE
#define CRYPT13_ALG_NONE    0x00
#define CRYPT13_ALG_DES     0x01
#define CRYPT13_ALG_DES3    0x02
#define CRYPT13_ALG_AES     0x03
#define CRYPT13_ALG_RC4     0x04
#define CRYPT13_ALG_XTEA    0x05
#define CRYPT13_ALG_NTRU    0x06
#define CRYPT13_ALG_BASE64  0x07
#define CRYPT13_ALG_ALL     0xff

#define CRYPT13_ALG_NULL    CRYPT13_ALG_NONE

#define CRYPT13_MODE_NONE   0x00
#define CRYPT13_MODE_STREAM 0x01
#define CRYPT13_MODE_BLOCK  0x02
#define CRYPT13_MODE_ALL    0xff

#define CRYPT13_TYPE_NONE   0x00
#define CRYPT13_TYPE_CIPHER 0x01
#define CRYPT13_TYPE_PUBKEY 0x02
#define CRYPT13_TYPE_CHANGE 0x03
#define CRYPT13_TYPE_ALL    0xff

#define CRYPT13_FLAG_HASPAYLOAD (0x01<<0)

struct crypt13{

    magic13_t magic;

    crypt13_alg_t alg;
    crypt13_mode_t mode;
    crypt13_type_t type;

    size_t blocksize;
    size_t keysize;

    crypt13_flag_t flags;

    char* name;
    char* desc;

    void* ctx;
    void* params;
};

char** crypt13_list_alg(crypt13_alg_t alg, crypt13_mode_t mode,
                        crypt13_type_t type);
error13_t crypt13_alg_info(crypt13_alg_t alg, struct crypt13* info);
error13_t crypt13_init( struct crypt13* s, crypt13_alg_t alg, uint8_t* key,
                        size_t keysize);
crypt13_alg_t crypt13_alg_id(char* name);
error13_t crypt13_destroy(struct crypt13* s);
error13_t crypt13_encrypt(  struct crypt13* s, uint8_t* input, size_t insize,
                            uint8_t* out, size_t* outsize);
error13_t crypt13_decrypt(struct crypt13* s, uint8_t *input, size_t insize,
                            uint8_t* out, size_t* outsize);

error13_t crypt13_pubkey(struct crypt13* s);
error13_t crypt13_pubkey(struct crypt13* s);

//size_t crypt13_bufsize(struct crypt13* s, size_t initsize);
size_t crypt13_enc_size(struct crypt13* s, uint8_t* buf, size_t initsize);
size_t crypt13_dec_size(struct crypt13* s, uint8_t* buf, size_t initsize);
size_t crypt13_export_pub_size(struct crypt13* s);
error13_t crypt13_export_pub(struct crypt13* s, char* buf);
error13_t crypt13_import_pub(struct crypt13* s, char* buf, size_t* size);

#endif // CRYPT13_H
