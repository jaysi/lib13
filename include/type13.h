#ifndef TYPE13_H
#define TYPE13_H

#include <sys/types.h>
#include <stdlib.h>     //needed? for size_t?
#include <stdint.h>		/* defines uint32_t etc */
#include <stddef.h>

#ifndef _STDINT_H

#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#ifndef int8_t
typedef char int8_t;
#endif
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif
#ifndef int16_t
typedef short int16_t;
#endif
#ifndef uint32_t
typedef unsigned long uint32_t;
#endif
#ifndef int32_t
typedef long int32_t;
#endif
#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif
#ifndef int64_t
typedef long long int64_t;
#endif

#endif

/*
#ifndef size_t
typedef unsigned int size_t;
#endif
*/

typedef uint64_t datalen13_t;
typedef uint16_t magic13_t;
typedef int error13_t;
typedef uint32_t reqid13_t;
typedef size_t msegid13_t;
typedef unsigned char uchar;
typedef uint32_t aclid13_t;
typedef aclid13_t gid13_t;
typedef aclid13_t uid13_t;
typedef uint32_t regid_t;
typedef uint16_t acc_perm_t;
typedef error13_t e13_t;

#endif // TYPE13_H
