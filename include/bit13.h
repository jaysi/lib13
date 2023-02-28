#ifndef BIT13_H
#define BIT13_H

#include "type13.h"

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/*
**  Macros to manipulate bits in an array of char.
**  These macros assume CHAR_BIT is one of either 8, 16, or 32.
*/

#define MASK  (CHAR_BIT-1)
#define SHIFT ((CHAR_BIT==8)?3:(CHAR_BIT==16)?4:5)

#define BITOFF(a,x)  ((void)((a)[(x)>>SHIFT] &= ~(1 << ((x)&MASK))))
#define BITON(a,x)   ((void)((a)[(x)>>SHIFT] |=  (1 << ((x)&MASK))))
#define BITFLIP(a,x) ((void)((a)[(x)>>SHIFT] ^=  (1 << ((x)&MASK))))
#define ISBIT(a,x)   ((a)[(x)>>SHIFT]        &   (1 << ((x)&MASK)))

#ifndef STRON
#define STRON "ON"
#endif

#ifndef STROFF
#define STROFF "OFF"
#endif

#define BITSTATSTR(bitbmap, bit) ((bit)&(bitbmap)?STRON:STROFF)

//bit bmaping macros

typedef uint32_t* bitmap13_t;
#ifndef BITMAPELEMENTSIZE
#define BITMAPELEMENTSIZE sizeof(uint32_t)
#endif

#define BITMAPSIZE(nseg) ((nseg)%((BITMAPELEMENTSIZE)*8))?((nseg)/(BITMAPELEMENTSIZE)+1):((nseg)/(BITMAPELEMENTSIZE))

#define BITMAPINDEX(n)              ((n)/BITMAPELEMENTSIZE)
#define BITMAPINDEXELEMENT(bmap, n) ((bmap)+BITMAPINDEX(n))
#define BITMAPBIT(n)                ((n)%BITMAPELEMENTSIZE)
#define BITMAPBITNO(bmap, index, bit_index) ((index)*BITMAPELEMENTSIZE*8+(bit_index))

#define BITMAPOFF(bmap, n)    BITOFF(BITMAPINDEXELEMENT(bmap, n), BITMAPBIT(n))
#define BITMAPON(bmap, n)     BITON(BITMAPINDEXELEMENT(bmap, n), BITMAPBIT(n))
#define BITMAPFLIP(bmap, n)   BITFLIP(BITMAPINDEXELEMENT(bmap, n), BITMAPBIT(n))
#define ISBITMAP(bmap, n)     ISBIT(BITMAPINDEXELEMENT(bmap, n), BITMAPBIT(n))

#define BITMAPBUF(buf, segsize, seg)    ((buf)+(((segsize)*(seg))/BITMAPELEMENTSIZE))

//the n is the total number of bits inside the bmap
//#define BITMAPFINDON(bmap, nbits)
//#define BITMAPFINDOFF(bmap, nbits)

#endif // BIT13_H
