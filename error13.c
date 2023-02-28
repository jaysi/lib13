#include <string.h>
#ifdef linux
#include <error.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "include/error13.h"
#include "include/str13.h"
#include "include/mem13.h"
#include "include/debug13.h"

#define _deb1   _NullMsg
#define _deb2   _DebugMsg

struct e13_code_strings{
    error13_t code;
    char* str;
} e13_strs[] = {
    {E13_OK, "Success"},
    {E13_ERROR, "Error"},
    {E13_INTERNAL, "Internal error"},
    {E13_PERM, "Access permission denied"},
    {E13_ABORT, "Callback routine requested an abort"},
    {E13_BUSY, "The resource is busy"},
    {E13_LOCKED, "The resource is locked"},
    {E13_NOMEM, "No memmory left"},
    {E13_READONLY, "Attempt to write a readonly resource"},
    {E13_INTERRUPT, "Operation terminated by an interrupt call"},
    {E13_IOERR, "I/O error"},
    {E13_CORRUPT, "The data is malformed"},
    {E13_NOTFOUND, "Not found"},
    {E13_FULL, "The resource is full"},
    {E13_CANTOPEN, "Unable to open the object"},
    {E13_PROTOCOL, "Protocol error"},
    {E13_EMPTY, "Empty"},
    {E13_CHANGE, "The resource has been changed"},
    {E13_TOOBIG, "Data exceeds size limit"},
    {E13_CONSTRAINT, "Constraint violation"},
    {E13_MISMATCH, "Data mismatch"},
    {E13_MISUSE, "Library used incorrectly"},
    {E13_NOLFS, "OS feature not supported on host"},
    {E13_AUTH, "Authorization denied"},
    {E13_FORMAT, "Format error"},
    {E13_RANGE, "Out of range"},
    {E13_NOTVALID, "Not a valid resource"},
    {E13_SYSE, "A System call has been failed"},
    {E13_EXISTS, "Already exists"},
    {E13_TIMEOUT, "Operation timed out"},
    {E13_DC, "Disconnected"},
    {E13_WRITEONLY, "Attempt to read from a writeonly object"},
    {E13_TOOSMALL, "Data exceeds lower size limit"},
    {E13_CONTINUE, "There is more data to process"},
    {E13_DONE, "Operation is done"},
    {E13_NEXT, "There is more data to process on next list item"},
    {E13_SYNTAX, "Syntax error"},
    {E13_IMPLEMENT, "Feature not implemented"},
    {-1, NULL}
};

#define _e13_is_init(h) (h)->magic==MAGIC13_E13?1:0

enum e13_string_code{
    E13_MSG_OK,
    E13_MSG_ALREADY_INIT,
    E13_MSG_MALLOC_FAILED
};

struct e13_string_list{
    enum e13_string_code code;
    char* str;
} e13_strings[] = {
    {E13_MSG_OK, "Success"},
    {E13_MSG_ALREADY_INIT, "Already initialized"},
    {E13_MSG_MALLOC_FAILED, "Memmory allocation failed"},
    {0, NULL}
};

static inline char* _e13_error_str(enum e13_string_code code){
    enum e13_string_code i = E13_MSG_OK;
    struct e13_string_list* strlist = e13_strings;

    while(strlist[i].str){
        if(code == strlist[i].code) return strlist[i].str;
        i++;
    }

    return NULL;
}

error13_t e13_init(struct e13* error_s, size_t max_warn, size_t max_err_str, enum lib13_src src){

	int i;

    if(!error_s) return E13_OK;

    if(_e13_is_init(error_s)){
        return e13_uerror(  error_s, E13_MISUSE, "s",
                            _e13_error_str(E13_MSG_ALREADY_INIT));
    }

    error_s->e13_code = E13_OK;

    error_s->max_warn = max_warn;
    error_s->max_err_str = max_err_str;

    error_s->ierr_str = (char*)m13_malloc(error_s->max_err_str);
    if(!error_s->ierr_str){
        return e13_error(E13_NOMEM);
    }

    error_s->uerr_str = (char*)m13_malloc(error_s->max_err_str);
    if(!error_s->uerr_str){
        m13_free(error_s->ierr_str);
        return e13_error(E13_NOMEM);
    }

    error_s->warn_code = (int*)m13_malloc(error_s->max_warn*sizeof(int));
    if(!error_s->warn_code){
        m13_free(error_s->ierr_str);
        m13_free(error_s->uerr_str);
        return e13_error(E13_NOMEM);
    }

//    error_s->warn_str = (char**)m13_malloc_2d(  error_s->max_warn + 1, 1,
//                                                error_s->max_err_str);

	error_s->warn_str = (char**)m13_malloc((error_s->max_warn)*sizeof(char*));

    if(!error_s->warn_str){
        m13_free(error_s->ierr_str);
        m13_free(error_s->uerr_str);
        m13_free(error_s->warn_code);
        return e13_error(E13_NOMEM);
    }

	for(i = 0; i < error_s->max_warn; i++){
		_deb1("i = %i", i);
		error_s->warn_str[i] = (char*)m13_malloc(error_s->max_err_str+1);
	}

    error_s->flags = 0;
    error_s->nwarn = 0UL;
    error_s->magic = MAGIC13_E13;
    error_s->src = src;

    error_s->next = NULL;

    return E13_OK;
}

error13_t e13_destroy(struct e13 *error_s){

	int i;

    if(!error_s) return E13_OK;

    if(!_e13_is_init(error_s)) return e13_error(E13_MISUSE);

    m13_free(error_s->ierr_str);
    m13_free(error_s->uerr_str);
    m13_free(error_s->warn_code);
//    m13_free_2d(error_s->warn_str);

	for(i = 0; i < error_s->max_warn; i++){
		m13_free(error_s->warn_str[i]);
	}
	m13_free(error_s->warn_str);

    error_s->magic = MAGIC13_INV;

    return E13_OK;
}

error13_t e13_cleanup(struct e13 *error_s){

    if(!error_s) return E13_OK;

    if(!_e13_is_init(error_s)) return e13_error(E13_MISUSE);

    error_s->flags = 0;
    error_s->nwarn = 0UL;
    error_s->e13_code = E13_OK;
    error_s->syse_code = 0;

    return E13_OK;
}


/*
 *the set functions always return the passed e13_code, this makes
 *programmer able to use return e13_set_*(); coding style;
*/

error13_t _e13_set_error(int which, struct e13* error_s, error13_t e13_code, char* format, ...){

    int i;
    size_t remain, thislen, copied = 0;

    if(!error_s) return e13_error(e13_code);

    remain = error_s->max_err_str;

    va_list vl;
    char* text;
    int intval;
    float f;
    char* buf;

    #define MAXNUMBUF 20

    char numbuf[MAXNUMBUF];

    if(error_s->magic != MAGIC13_E13) return e13_error(E13_MISUSE);

    switch(which){
        case 0:
            buf = error_s->uerr_str;
            error_s->flags |= E13_FLAG_UERR;
            error_s->e13_code = e13_error(e13_code);
        break;
        case 1:
            buf = error_s->ierr_str;
            error_s->flags |= E13_FLAG_IERR;
            error_s->e13_code = e13_error(e13_code);
        break;
        default:
            if(error_s->nwarn == error_s->max_warn){
                error_s->nwarn = 1;
            } else {
                error_s->nwarn++;
            }
            _deb1("nwarn = %i, maxwarn = %i",error_s->nwarn,error_s->max_warn);
            buf = error_s->warn_str[error_s->nwarn - 1];
            assert(buf);
            error_s->warn_code[error_s->nwarn - 1] = e13_error(e13_code);
            error_s->flags |= E13_FLAG_WARN;
        break;
    }

    va_start(vl, format);

    for(i = 0; format[i] != '\0'; ++i){

        switch(format[i]){

            case 'i':
            intval = va_arg(vl, int);

            snprintf(numbuf, MAXNUMBUF, "%i", intval);

            thislen = strlen(numbuf);

            if(thislen + copied > remain) goto end;

            snprintf(   buf + copied,
                        remain,
                        "%s", numbuf);
            break;

            case 'f':
            f = va_arg(vl, double);

            snprintf(numbuf, MAXNUMBUF, "%f", f);

            thislen = strlen(numbuf);

            if(thislen + copied > remain) goto end;

            snprintf(   buf + copied,
                        remain,
                        "%s", numbuf);
            break;

            case 's':
            text = va_arg(vl, char*);

            thislen = strlen(text);

            if(thislen + copied >= remain) goto end;

            _deb1("thislen = %i, copied = %i, remain = %i, text = %s, buf = %s",
				thislen, copied, remain, text, buf);

            snprintf(   buf + copied,
                        remain,
                        "%s", text);
            break;

        default:

            if(thislen + copied > remain) goto end;

            snprintf(   buf + copied,
                        remain,
                        "%c", format[i]);
            break;
        }

        copied += thislen;
        remain -= thislen;
    }

    va_end(vl);

end:

	_deb2("e13 buf: %s", buf);
    return error_s->e13_code;
}

error13_t e13_copy(struct e13* dst, struct e13* src){
    if(!_e13_is_init(dst) || !_e13_is_init(src)) return e13_error(E13_MISUSE);
    memcpy(dst, src, sizeof(struct e13));
    return E13_OK;
}

char* e13_codemsg(error13_t code){
    struct e13_code_strings* e13_str;

    if(code < 0) code *= -1;
    _deb1("codemsg: %i", code);

    for(e13_str = e13_strs; e13_str->str; e13_str++)
        if(e13_str->code == code) return e13_str->str;
    return "Unknown error";
}
