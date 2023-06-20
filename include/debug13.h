#ifndef DEBUG13_H
#define DEBUG13_H

#include <stdio.h>
#include <stdlib.h>

#include "const13.h"

#define DEBUGLEVEL

#ifndef NDEBUG

#define _DEBUG13_MAX_STR 100

/*
static char _Debug13Buf[_DEBUG13_MAX_STR];

#ifdef _WIN32
#define _DebugMsg(fmt, args...) MACRO(\
    snprintf(_Debug13Buf, _DEBUG13_MAX_STR, "dbg{%s::%i# ", __FILE__, __LINE__);\
    snprintf(_Debug13Buf+strlen(_Debug13Buf), _DEBUG13_MAX_STR-strlen(_Debug13Buf), fmt, ## args);\
    snprintf(_Debug13Buf+strlen(_Debug13Buf), _DEBUG13_MAX_STR-strlen(_Debug13Buf), "}dbg;\n");\
    perror(_Debug13Buf);)
#else
*/

#define _DebugMsg(fmt, ... )    MACRO( fprintf(stderr, "dbg{%s:%s:%i# ", __FILE__, __func__, __LINE__);\
                                    fprintf(stderr, fmt, ##);\
                                    fprintf(stderr, " }dbg;\n"); )
//#endif
#define _DebMsgL(level, fmt, ... )  MACRO( if(level >= DEBUGLEVEL){\
                                                _DebugMsg(fmt, ##);} )
#define _AssertMacro(if_bool)   MACRO( if(!(if_bool)){_DebugMsg("### ASSERT FAILED!!! ANY KEY TO CONTINUE. ###");getch();} )
#define _GetKey()   MACRO( getch(); )
#else

#define _DebugMsg(fmt, ... )
#define _DebMsgL(level, fmt, ... )
#define _AssertMacro(if_bool)

#endif

#define _NullMsg(fmt, ... )

#define _FatalMsg(fmt, ... )    MACRO( printf("fatal{%s:%s:%i# ", __FILE__, __func__, __LINE__);\
                                    printf(fmt, ##);\
                                    printf(" }fatal;\n"); )

#define _Fatal(fmt, ... ) MACRO( _FatalMsg(fmt, ##); exit(255) )

#endif // DEBUG13_H
