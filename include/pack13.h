#ifndef _PACK13_H
#define _PACK13_H

#include <sys/types.h>
#include "base64.h"

/*
    actually the whole packing comes from beej,
    i just changed it a bit to use inlining in my code...
*/

/*
** pack() -- store data dictated by the format string in the buffer
**
**   bits |signed   unsigned   float   string	buffer
**   -----+-------------------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s		b(need 2 args)
**
**  (16-bit unsigned length is automatically prepended to strings and buffers)
*/

size_t pack13(unsigned char *buf, size_t bufsize, char *format, ...);
size_t unpack13(unsigned char *buf, size_t bufsize, char *format, ...);

#define pack754_16(f) (pack754((f), 16, 5))
#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_16(i) (unpack754((i), 16, 5))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

//base 64 encode
#define pack13_b64(dst, dest, len) b64enc(dst, dest, len)

#else

#endif
