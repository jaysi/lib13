
#ifndef _ERROR13_H
#define _ERROR13_H

#include <sys/types.h>

#include "bit13.h"
#include "type13.h"
#include "const13.h"

#define E13_OK           0   /* Successful result */
/* beginning-of-error-codes */
#define E13_ERROR        10001   /* Error or missing */
#define E13_INTERNAL     10002   /* Internal error */
#define E13_PERM         10003   /* Access permission denied */
#define E13_ABORT        10004   /* Callback routine requested an abort */
#define E13_BUSY         10005   /* The object is busy */
#define E13_LOCKED       10006   /* The object is locked */
#define E13_NOMEM        10007   /* No memmory left! */
#define E13_READONLY     10008   /* Attempt to write a readonly object */
#define E13_INTERRUPT    10009   /* Operation terminated by an interrupt call */
#define E13_IOERR       100010   /* Some kind of disk or net I/O error occurred */
#define E13_CORRUPT     100011   /* The data is malformed */
#define E13_NOTFOUND    100012   /* Not found */
#define E13_FULL        100013   /* The resource is full */
#define E13_CANTOPEN    100014   /* Unable to open the object */
#define E13_PROTOCOL    100015   /* Protocol error */
#define E13_EMPTY       100016   /* The resource is empty */
#define E13_CHANGE      100017   /* The object has been changed */
#define E13_TOOBIG      100018   /* Data exceeds size limit */
#define E13_CONSTRAINT  100019   /* Abort due to constraint violation */
#define E13_MISMATCH    100020   /* Data type mismatch */
#define E13_MISUSE      100021   /* Library used incorrectly */
#define E13_NOLFS       100022   /* Uses OS features not supported on host */
#define E13_AUTH        100023   /* Authorization denied */
#define E13_FORMAT      100024   /* Format error */
#define E13_RANGE       100025   /* Out of range */
#define E13_NOTVALID    100026   /* Resource opened that is not a valid resource */
#define E13_SYSE        100027   /* A System call has been failed */
#define E13_EXISTS      100028   /* the resource already exists */
#define E13_TIMEOUT     100029   /* timed out */
#define E13_DC          100030   /* disconnected */
#define E13_WRITEONLY   100031   /* Attempt to read from a writeonly object */
#define E13_TOOSMALL    100032   /* Data exceeds lower size limit */
#define E13_CONTINUE    1000100  /* There is more data to process */
#define E13_DONE        1000101  /* No more data to process */
#define E13_NEXT        1000102  /* There is more data to process on next list item */
#define E13_SYNTAX		1000103	 /* Syntax error*/

#define E13_IMPLEMENT   1000200  /* Feature not implemented */

/* end-of-error-codes */

#define E13_FLAG_IERR   (0x0001<<0)
#define E13_FLAG_UERR   (0x0001<<1)
#define E13_FLAG_WARN   (0x0001<<2)
#define E13_FLAG_ESYS   (0x0001<<3)

#define E13_MAX_WARN_DEF    10
#define E13_MAX_ESTR_DEF    256

struct e13{

    magic13_t magic;

    short flags;

    //config
    enum lib13_src src;
    size_t max_warn;
    size_t max_err_str;

    int e13_code;
    int syse_code;

    size_t nwarn;
    int* warn_code;

    char* ierr_str; //internal error string, used for developement, debugging
    char* uerr_str; //user error string
    char** warn_str;//warning string array

    struct e13* next;//this enables the lib13 to handle error lists.

};

#ifdef __cplusplus
    extern "C" {
#endif

error13_t e13_init(struct e13* error_s, size_t max_warn, size_t max_err_str, enum lib13_src src);

error13_t _e13_set_error(int which, struct e13* error_s, error13_t e13_code, char* format, ...);
#define e13_uerror(e13, code, fmt, ...) _e13_set_error(0, e13, code, fmt, __VA_ARGS__)
#define e13_ierror(e13, code, fmt, ...) _e13_set_error(1, e13, code, fmt, __VA_ARGS__)
#define e13_warn(e13, code, fmt, ...) _e13_set_error(2, e13, code, fmt, __VA_ARGS__)
//#define e13_uerror(e13, code, fmt, args...) _e13_set_error(0, e13, code, fmt, ## args)
//#define e13_ierror(e13, code, fmt, args...) _e13_set_error(1, e13, code, fmt, ## args)
//#define e13_warn(e13, code, fmt, args...) _e13_set_error(2, e13, code, fmt, ## args)

char** e13_warnmsg(struct e13* error_s);
char* e13_codemsg(error13_t code);

//error13_t e13_errmsg(struct e13* e);
#define e13_uerrmsg(e13) ((e13)->flags&E13_FLAG_UERR?(e13)->uerr_str:"No Error")
#define e13_ierrmsg(e13) ((e13)->flags&E13_FLAG_IERR?(e13)->ierr_str:"No Error")


error13_t e13_cleanup(struct e13* error_s);//prepare error_s struct for another use
error13_t e13_destroy(struct e13* error_s);

error13_t e13_copy(struct e13* dst, struct e13* src);

#define e13_error(e13_eno) ((e13_eno)>0)?((-1)*(e13_eno)):(e13_eno)
#define e13_errcode(e13) ((e13)->e13_code)
#define e13_iserror(e13) (e13<0?1:0)

#ifdef __cplusplus
    }
#endif


#endif
