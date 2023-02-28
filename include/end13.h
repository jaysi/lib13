#ifndef END13_H
#define END13_H

#include <sys/param.h>		/* attempt to define endianness */
#ifdef linux
#include <endian.h>		/* attempt to define endianness */
#endif

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
#define LIB13_LITTLE_ENDIAN 1
#define LIB13_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
#define LIB13_LITTLE_ENDIAN 0
#define LIB13_BIG_ENDIAN 1
#else
#define LIB13_LITTLE_ENDIAN 0
#define LIB13_BIG_ENDIAN 0
#endif

#endif // END13_H
