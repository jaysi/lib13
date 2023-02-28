#ifndef CONST13_H
#define CONST13_H

#include "type13.h"

#ifdef MACRO
#undef MACRO
#endif
#define MACRO(A) do { A; } while(0)

/*      MAGIC NUMBERS       */
#define MAGIC13_AC13  0xac13
#define MAGIC13_E13   0xe013
#define MAGIC13_NM    0x2e43
#define MAGIC13_NMDB  0x23db
#define MAGIC13_DB13  0xdb13
#define MAGIC13_M13   0x3e30
#define MAGIC13_RR13  0x1c1c
#define MAGIC13_CR13  0xecdc
#define MAGIC13_SS13  0xd1e4
#define MAGIC13_IO13  0x1013
#define MAGIC13_LK13  0x1130
#define MAGIC13_LI13  0x1113
//#define MAGIC13_FB    0xf1ab
#define MAGIC13_COMPAS 0xc035
#define MAGIC13_OBJ13 0x0b13
#define MAGIC13_ILINK 0x111c
#define MAGIC13_MONET 0x302e
#define MAGIC13_INV   0x184e

#ifndef true_
#define true_ (1)
#endif

#ifndef false_
#define false_ (0)
#endif

enum lib13_src{
    LIB13_ERROR,
    LIB13_NETMETRE,
    LIB13_NMDB,
    LIB13_DB,
    LIB13_MEM,
    LIB13_RR,
    LIB13_CRYPT,
    LIB13_IO,
    LIB13_LOCK,
    LIB13_LIB,
    LIB13_COMPASS,
    LIB13_OBJ,
    LIB13_ILINK,
    LIB13_MONET,
    LIB13_ARG,
    LIB13_INV
};

#endif // CONST13_H
