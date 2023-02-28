#ifndef __STR13_H
#define __STR13_H

#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include "error13.h"
#include "mem13.h"

#define BASEB   1024
#define KB      (BASEB*1)
#define MB      (BASEB*KB)
#define GB      (BASEB*MB)
#define TB      (BASEB*GB)
#define EB      (BASEB*TB)

#define S13_PROGLIST_CHUNK_DEF  1024
#define S13_PROGLIST_STRLEN_MAX 1024
#define S13_PROGLIST_DELIM_DEF  ';'

struct  s13_proglist{
    char* buf;
    size_t chunk;
    size_t bufsize;
    size_t pos;
    char delim;
};

#ifdef __cplusplus
    extern "C" {
#endif

//int s13_exparts(char* str, char* delim, char escape);
//int s13_explode(char* str, char* delim, char escape, char** ary);
//char* s13_prepare_exparts(char* str, char* delim);
char* s13_find_expart(char* str, char* delim, char escape, char* search);
int s13_merge_exparts(char* set1, char* set2, char** pack, char* delim, char escape, char* buf);
char* s13_join_array(char** array, char* delim, char escape, size_t narray);
size_t s13_array_size(char** array, size_t n);
char** s13_copy_array(char** array, size_t narray);

#define s13_exmem(n) (char**)malloc((n)*sizeof(char**))
#define s13_free_exmem(ptr) free(ptr)

//argument parse,
int s13_explode(char* arg, char* delim, char** pack, char escape, char** ary);
int s13_exparts(char* str, char* delim, char** pack, char escape);

size_t s13_strlen(const char *str, size_t max_len);
size_t s13_wcslen(const wchar_t * str, size_t max_len);
size_t s13_strcpy(char *dest, const char *src, size_t size);
size_t s13_strcat(char *dest, const char *src, size_t count);
size_t s13_wcscpy(wchar_t * dest, const wchar_t * src, size_t size);
size_t s13_wcscat(wchar_t * dest, const wchar_t * src, size_t count);
char* s13_malloc_strcpy(const char* src, size_t size);
char* s13_malloc_strcat(const char* src1, const char* src2, size_t size);

int s13_wildcmp(const char* dst, const char* src, const char any,
                const char one, const char escape);

error13_t nt_convert(long double val, char* buf, size_t bufsize);
error13_t nt_convert_str(char* numbuf, char* buf, size_t bufsize);

#ifdef __cplusplus
    }
#endif

#else

#endif
