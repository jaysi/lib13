/* $Id$ */
#ifndef __BASE64_H
#define __BASE64_H

#include "type13.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t base64encode_size(const void* data_buf, size_t dataLength);
size_t base64decode_size(char *in, size_t inLen);
int base64encode(const void* data_buf, size_t dataLength, char* result, size_t* resultSize);
int base64decode (char *in, size_t inLen, unsigned char *out, size_t *outLen);


#ifdef __cplusplus
}
#endif

#endif//__BASE64_H

