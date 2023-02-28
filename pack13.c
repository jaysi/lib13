#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "include/pack13.h"
#include "include/error13.h"
#include "include/debug13.h"
#include "include/pack13i.c"

#define _debu _NullMsg

#define ISROOM() MACRO( if(size > bufsize){_debu("size %u > bufsize %u", size, bufsize); return ((size_t)-1);} )

/*
    from beej
*/

/*
** pack() -- store data dictated by the format string in the buffer
**
**   bits |signed   unsigned   float   string   buffer
**   -----+------------------------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s         b(2 args)
**
**  (16-bit unsigned length is automatically prepended to strings)
*/

size_t pack13(unsigned char *buf, size_t bufsize, char *format, ...)
{
    va_list ap;

    signed char c;              // 8-bit
    unsigned char C;

    int h;                      // 16-bit
    unsigned int H;

    long int l;                 // 32-bit
    unsigned long int L;

    long long int q;            // 64-bit
    unsigned long long int Q;

    float f;                    // floats
    double d;
    long double g;
    unsigned long long int fhold;

    char *s;                    // strings
    unsigned int len;

    size_t size = 0;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {
        case 'c': // 8-bit
            size += 1;
            ISROOM();
            c = (signed char)va_arg(ap, int); // promoted
            *buf++ = c;
            break;

        case 'C': // 8-bit unsigned
            size += 1;
            ISROOM();
            C = (unsigned char)va_arg(ap, unsigned int); // promoted
            *buf++ = C;
            break;

        case 'h': // 16-bit
            size += 2;
            ISROOM();
            h = va_arg(ap, int);
            packi16(buf, h);
            buf += 2;
            break;

        case 'H': // 16-bit unsigned
            size += 2;
            ISROOM();
            H = va_arg(ap, unsigned int);
            packi16(buf, H);
            buf += 2;
            break;

        case 'l': // 32-bit
            size += 4;
            ISROOM();
            l = va_arg(ap, long int);
            packi32(buf, l);
            buf += 4;
            break;

        case 'L': // 32-bit unsigned
            size += 4;
            ISROOM();
            L = va_arg(ap, unsigned long int);
            packi32(buf, L);
            buf += 4;
            break;

        case 'q': // 64-bit
            size += 8;
            ISROOM();
            q = va_arg(ap, long long int);
            packi64(buf, q);
            buf += 8;
            break;

        case 'Q': // 64-bit unsigned
            size += 8;
            ISROOM();
            Q = va_arg(ap, unsigned long long int);
            packi64(buf, Q);
            buf += 8;
            break;

        case 'f': // float-16
            size += 2;
            ISROOM();
            f = (float)va_arg(ap, double); // promoted
            fhold = pack754_16(f); // convert to IEEE 754
            packi16(buf, fhold);
            buf += 2;
            break;

        case 'd': // float-32
            size += 4;
            ISROOM();
            d = va_arg(ap, double);
            fhold = pack754_32(d); // convert to IEEE 754
            packi32(buf, fhold);
            buf += 4;
            break;

        case 'g': // float-64
            size += 8;
            ISROOM();
            g = va_arg(ap, long double);
            fhold = pack754_64(g); // convert to IEEE 754
            packi64(buf, fhold);
            buf += 8;
            break;

        case 's': // string
            s = va_arg(ap, char*);
            len = strlen(s);
			if(!len) return ((size_t)-1);
            size += len + 2;
            ISROOM();
            packi16(buf, len);
            buf += 2;
            memcpy(buf, s, len);
            buf += len;
            break;

		case 'b':// buffer
            len = va_arg(ap, unsigned int);
            if(!len){
				_debu("fails !len");
				return ((size_t)-1);
            }
            _debu("len = %u", len);
			s = va_arg(ap, char*);
			size += len + sizeof(unsigned int);
			ISROOM();
            packi16(buf, len);
            buf += 2;
            memcpy(buf, s, len);
            buf += len;
			break;

        }
    }

    va_end(ap);

    return size;
}

/*
** unpack() -- unpack data dictated by the format string into the buffer
**
**   bits |signed   unsigned   float   string   buffer
**   -----+------------------------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s         b(2 args)
**
**  (string is extracted based on its stored length, but 's' can be
**  prepended with a max length)
*/
size_t unpack13(unsigned char *buf, size_t bufsize, char *format, ...)
{
    va_list ap;

    signed char *c;              // 8-bit
    unsigned char *C;

    int *h;                      // 16-bit
    unsigned int *H;

    long int *l;                 // 32-bit
    unsigned long int *L;

    long long int *q;            // 64-bit
    unsigned long long int *Q;

    float *f;                    // floats
    double *d;
    long double *g;
    unsigned long long int fhold;

    char *s;
    unsigned int len, maxstrlen=0, count;

    size_t size = 0;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {
        case 'c': // 8-bit
            size += 1;
            ISROOM();
            c = va_arg(ap, signed char*);
            if (*buf <= 0x7f) { *c = *buf++;} // re-sign
            else { *c = -1 - (unsigned char)(0xffu - *buf); }
            break;

        case 'C': // 8-bit unsigned
            size += 1;
            ISROOM();
            C = va_arg(ap, unsigned char*);
            *C = *buf++;
            break;

        case 'h': // 16-bit
        	size += 2;
        	ISROOM();
            h = va_arg(ap, int*);
            *h = unpacki16(buf);
            buf += 2;
            break;

        case 'H': // 16-bit unsigned
        	size += 2;
        	ISROOM();
            H = va_arg(ap, unsigned int*);
            *H = unpacku16(buf);
            buf += 2;
            break;

        case 'l': // 32-bit
        	size += 4;
        	ISROOM();
            l = va_arg(ap, long int*);
            *l = unpacki32(buf);
            buf += 4;
            break;

        case 'L': // 32-bit unsigned
        	size += 4;
        	ISROOM();
            L = va_arg(ap, unsigned long int*);
            *L = unpacku32(buf);
            buf += 4;
            break;

        case 'q': // 64-bit
        	size += 8;
        	ISROOM();
            q = va_arg(ap, long long int*);
            *q = unpacki64(buf);
            buf += 8;
            break;

        case 'Q': // 64-bit unsigned
        	size += 8;
        	ISROOM();
            Q = va_arg(ap, unsigned long long int*);
            *Q = unpacku64(buf);
            buf += 8;
            break;

        case 'f': // float
        	size += 2;
        	ISROOM();
            f = va_arg(ap, float*);
            fhold = unpacku16(buf);
            *f = unpack754_16(fhold);
            buf += 2;
            break;

        case 'd': // float-32
        	size += 4;
        	ISROOM();
            d = va_arg(ap, double*);
            fhold = unpacku32(buf);
            *d = unpack754_32(fhold);
            buf += 4;
            break;

        case 'g': // float-64
        	size += 8;
        	ISROOM();
            g = va_arg(ap, long double*);
            fhold = unpacku64(buf);
            *g = unpack754_64(fhold);
            buf += 8;
            break;

        case 's': // string
            s = va_arg(ap, char*);
            len = unpacku16(buf);
            size += len + 2;
            //assert
			if(!len) return ((size_t)-1);
            ISROOM();
            buf += 2;
            if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
            else count = len;
            memcpy(s, buf, count);
            s[count] = '\0';
            buf += len;
            break;

        case 'b': //buffer
        	H = va_arg(ap, unsigned int*);
        	len = *H;
        	//memcpy(H, &len, sizeof(unsigned int));
        	s = va_arg(ap, char*);
        	*H = unpacku16(buf);
        	//assert
			if(!(*H) || len < *H){
					_debu("len %u *H %u", len, *H);
					return ((size_t)-1);
			}
        	size += (*H) + 2;
        	ISROOM();
        	buf += 2;
            memcpy(s, buf, (*H));
            //if(*H > len) buf[len] = 0;
            buf += (*H);
            break;

        default:
            if (isdigit(*format)) { // track max str len
                maxstrlen = maxstrlen * 10 + (*format-'0');
            }
            break;
        }

        if (!isdigit(*format)) maxstrlen = 0;
    }

    va_end(ap);

    return size;
}
