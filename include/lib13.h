#ifndef LIB13_H
#define LIB13_H

#define LIB13_TEST

/*
 * 32bits
 * from left:
 * byte0:
 * byte1:
 * byte2:
 * byte3:
*/
#define LIB13_VERSION   0x00000001

#include "acc13.h"
#include "arg13.h"
#include "bit13.h"
#include "const13.h"
#include "crypt13.h"
//#include "csv13.h"
#include "day13.h"
#include "debug13.h"
//#include "ds13.h"
#include "end13.h"
#include "error13.h"
#include "hash13.h"
#include "io13.h"
#include "lock13.h"
#include "mem13.h"
#include "pack13.h"
#include "path13.h"
#include "rr13.h"
#include "str13.h"
#include "thread13.h"
#include "type13.h"
#include "db13.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t lib13_ver();

#ifdef __cplusplus
}
#endif

#endif // LIB13_H
