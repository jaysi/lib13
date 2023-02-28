#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "include/str13.h"
#include "include/debug13.h"

#undef TEST
//#define TEST

//#define _deb1 _DebugMsg
#define _deb1 _NullMsg

/* the whole thing was crap!
char* s13_prepare_exparts(char* str, char* delim){

    char* start;
    char* end;
    register size_t dlen = strlen(delim);

    start = str;
    end = str + strlen(str);

    while(!memcmp(start, delim, dlen) && strlen(start)) start += dlen;
    while(!memcmp(end - dlen, delim, dlen) && strlen(start)) {end -= dlen; *end = '\0';};

    return start;
}
*/

/*
static inline void _s13_fix_last_delim(char* str, int slen, char* delim, int dlen){

    if(!memcmp(str + slen - dlen, delim, dlen)) memset(str + slen - dlen, '\0', dlen);

    _deb1("fixed as %s", str);

}
*/

/*
int s13_exparts(char* str, char* delim){
    int ret = 0;
    char* tmp = str;
    int dlen = strlen(delim);
    int slen = strlen(str);

    if(!slen) return 0;

    do{
        if((tmp = strstr(tmp, delim))){
                tmp += dlen;
        }
        ret++;
    }while(tmp);

    if(!memcmp(str + slen - dlen, delim, dlen)) ret--;
    return ret;
}

int s13_explode(char* str, char* delim, char** ary){

    register int i = 0, dlen = strlen(delim);
    char* tmp;
    int slen = strlen(str);

    ary[i] = str;
    tmp = str;

    _s13_fix_last_delim(str, slen, delim, dlen);

    do{
        if(!(tmp = strstr(tmp, delim))) break;

        memset(tmp, '\0', dlen);
        i++;
        tmp += dlen;
        ary[i] = tmp;

    }while(tmp);

    return ++i;
}
*/



//int s13_wildcmp(const char* dst, const char* src, const char any, const char one){

//	while(*src){
//		//printf("> %c, %c\n", *dst, *src);
//		if(*src == any || *src == '\0');
//		else if(*src == one) { dst++; }
//		else if(!(dst = strchr(dst, *src))) return -1;
//		/* OLD
//		switch(*src){
//            case any:
//			break;
//            case one:
//				dst++;
//			break;
//			case '\0':
//			break;
//			default:
//				if(!(dst = strchr(dst, *src))) return -1;
//			break;
//		}
//		*/
//		src++;
//		//printf("< %c, %c\n", *dst, *src);
//	}
//
//	//now *src is finished, check the dst
//	dst++;
//	src -= 2;
//	if(	*dst &&
//		*src != *dst &&
//        *src != any
//		){
//        if((*src == one && strlen(dst) == 1)) return 0;
//		return -1;
//	}

//	return 0;
//}

static inline void _s13_fix_last_delim(char* str, int slen, char* delim, int dlen, char escape){

    if(!memcmp(str + slen - dlen, delim, dlen))
        if(*(str+slen-dlen-1) != escape)
            memset(str + slen - dlen, '\0', dlen);

    _deb1("fixed as %s", str);

}

char* s13_find_expart(char* str, char* delim, char escape, char* search){

    register size_t exlen, slen = strlen(search);
    char* start = str;
    char* end = start;

    while((end = strstr(start, delim))){
        exlen = end - start;
        if(exlen != slen){
            start = end + 1;
            continue;
        }
        if(!memcmp(start, search, exlen)){
            return start;
        }
        start = end + 1;
    }

    if(!memcmp(str + strlen(str) - slen, search, slen)) return str + strlen(str) - slen;

    return NULL;

}

int s13_merge_exparts(char* set1, char* set2, char** pack, char* delim, char escape, char* buf){
    register int n2;
    char* s1, *s2;
    char **a2;

    _s13_fix_last_delim(set1, strlen(set1), delim, strlen(delim), escape);
    _s13_fix_last_delim(set2, strlen(set2), delim, strlen(delim), escape);

    s1 = set1;
    s2 = set2;

    strcpy(buf, s1);
    strcat(buf, delim);

    n2 = s13_exparts(set2, delim, pack, escape);

    if(n2) a2 = s13_exmem(n2);
    else return 0;

    s13_explode(s2, delim, pack, escape, a2);

    while(n2--){
        if(!s13_find_expart(s1, delim, escape, a2[n2])){
            strcat(buf, a2[n2]);
            strcat(buf, delim);
        }
    }

    s13_free_exmem(a2);

    return 0;
}

static inline void _s13_restore_escapes(char* buf, char* delim, char escape){

    char* needle = buf;
    while((needle = strstr(needle, delim))){
        _deb1("copying %s to %s", needle, needle+1);
        strcpy(needle+1, needle);
        *needle = escape;
    }

    _deb1("restored as %s", buf);

}

char* s13_join_array(char** array, char* delim, char escape, size_t narray){
    char* buf, *needle;
    register size_t bufsize = 0, i;
    int dlen = strlen(delim);

    for(i = 0; i < narray; i++){

        bufsize += strlen(array[i]) + dlen;

        _deb1("bufsize before escapes = %i, array[%i]=%s", bufsize, i, array[i]);

        if(escape){
            needle = array[i];
            _deb1("needle = %s", needle);
            while((needle = strstr(needle, delim))){
                needle++;
                bufsize++;
            }
        }

        _deb1("bufsize after escapes = %i", bufsize);

    }
    _deb1("array[0]=%s", array[0]);

    bufsize++;//for end nul

    _deb1("bufsize=%i", bufsize);

    buf = (char*)malloc(bufsize);
    if(!buf) return NULL;

    _deb1("array[0]=%s", array[0]);

    strcpy(buf, array[0]);
    _deb1("buf=%s, array[0]=%s", buf, array[0]);
    _s13_restore_escapes(buf, delim, escape);
    needle = buf + strlen(buf);
    for(i = 1; i < narray; i++){

        strcpy(needle, delim);
        needle += dlen;
        strcpy(needle, array[i]);
        _s13_restore_escapes(needle, delim, escape);
        needle += strlen(needle);

    }

    return buf;
}

size_t s13_array_size(char** array, size_t n){
    size_t bufsize = 0UL;
    size_t i;
    for(i = 0; i < n; i++)
        bufsize += strlen(array[i]) + 1;

    return bufsize;
}

char** s13_copy_array(char** array, size_t narray){
    char** buf, *pos;
    register size_t bufsize = s13_array_size(array, narray);
    size_t i;

    _deb1("bufsize=%u, *array=%s", bufsize, *array);

    buf = (char**)malloc((narray)*sizeof(char**)+bufsize);
    if(!buf) return NULL;

    pos = (char*)(buf + narray*sizeof(char**));
    for(i = 0; i < narray; i++){
        buf[i] = pos;
        strcpy(buf[i], array[i]);
        pos += strlen(array[i])+1;
    }

    return buf;
}

/*
 * argument parsers
 * valid shape: part1;part2;"part3a;part3b";part4;...
*/

int s13_exparts(char* str, char* delim, char** pack, char escape){

    char* needle = NULL;
    int dlen = strlen(delim), nlen;
    int slen = strlen(str);
    int j;

    int ret = 0;
    char* tmp = str;

    if(!slen) return 0;

    do{
        if(!needle){
            if((tmp = strstr(tmp, delim))){

                if(escape && tmp > str && *(tmp-1) == escape){
                    tmp += dlen;
                    continue;
                }

                tmp+=dlen;

                if(pack){
                    j = 0;
                    while(pack[j]){
                        if(!memcmp(pack[j], tmp, strlen(pack[j]))){

                            if(*(tmp-1) == escape){
                                tmp += strlen(pack[j]);
                                continue;
                            }

                            needle = pack[j];
                            nlen = strlen(needle);
                        }
                        j++;
                    }
                }

            }
            ret++;
        } else {
            if((tmp = strstr(tmp, needle))){

                if(!memcmp(tmp+nlen, delim, dlen)){

                    if(escape && *(tmp-1) == escape){
                        tmp += nlen;
                        continue;
                    }

                    needle = NULL;
                }
                tmp += nlen;

            }
        }
    }while(tmp);

    if(	!memcmp(str + slen - dlen, delim, dlen) &&
        *(str + slen - dlen - 1) != escape) ret--;
    return ret;

}

int s13_explode(char* arg, char* delim, char** pack, char escape, char** ary){
    register int i = 0, dlen = strlen(delim), j, nlen;
    char* tmp;
    char* needle = NULL;
    int slen = strlen(arg);

    ary[i] = arg;
    tmp = arg;

    _s13_fix_last_delim(arg, slen, delim, dlen, escape);

    do{
        if(!needle){

            if(!(tmp = strstr(tmp, delim))) break;

            /* the escape character is useful
             * just before delim or pack, so \; or \"
             * must be translated to ; and "
            */
            if(escape && tmp > arg && *(tmp-1) == escape){
                //eliminate \ before ;
                strcpy(tmp-1, tmp);
                tmp += dlen;
                continue;
            }

            memset(tmp, '\0', dlen);
            i++;
            tmp += dlen;

            if(pack){
                j = 0;
                while(pack[j]){
                    if(!memcmp(pack[j], tmp, strlen(pack[j]))){

                        if(*(tmp-1) == escape){//no crash? check bounds
                            //*(tmp-1) = '\0';
                            strcpy(tmp-1, tmp);
                            tmp += strlen(pack[j]);
                            continue;
                        }

                        needle = pack[j];
                        nlen = strlen(needle);
                        tmp += nlen;
                        break;
                    }
                    j++;
                }
            }

            ary[i] = tmp;

        } else {

            if(!(tmp = strstr(tmp, needle))) break;//now search for the needle

            if(escape && *(tmp-1) == escape){
                strcpy(tmp-1, tmp);//shift back
                tmp += nlen;
                continue;
            }

            if(!memcmp(tmp+nlen, delim, dlen)){
                needle = NULL;
                memset(tmp, '\0', nlen);
            }
            tmp += nlen;
        }

    }while(tmp);

    return ++i;
}

size_t s13_strlen(const char *str, size_t max_size)
{
    size_t ret;
    for (ret = 0; ret < max_size; ret++)
        if (str[ret] == '\0')
            break;

    return ret;
}

size_t s13_wcslen(const wchar_t * str, size_t max_size)
{
    size_t ret;
    for (ret = 0; ret < max_size; ret++)
        if (str[ret] == L'\0')
            break;

    return ret;
}

size_t s13_strcpy(char *dest, const char *src, size_t size)
{
    size_t ret = s13_strlen(src, size);
    size_t len = (ret >= size) ? size - 1 : ret;

    assert(dest);

    if (size) {
        memcpy(dest, src, len);
        dest[len] = '\0';
    }
    return ret;
}

char* s13_malloc_strcpy(const char* src, size_t size){

    char* dst;

    dst = (char*)m13_malloc(s13_strlen(src, size) + 1);
    if(!dst) return NULL;
    s13_strcpy(dst, src, size + 1);
    return dst;

}

char* s13_malloc_strcat(const char* src1, const char* src2, size_t size){

    char* dst;
    size_t len = s13_strlen(src1, size) + s13_strlen(src2, size) + 1;
    if(len > size) len = size + 1;
    dst = (char*)m13_malloc(len);
    if(!dst) return NULL;

    s13_strcpy(dst, src1, len);
    len = s13_strlen(dst, len);
    s13_strcat(dst, src2, len);

    return dst;

}

size_t s13_strcat(char *dest, const char *src, size_t count)
{
    size_t dsize = s13_strlen(dest, count);
    size_t len = s13_strlen(src, count);
    size_t res = dsize + len;

    assert(dest);

    dest += dsize;
    count -= dsize;
    if (len >= count)
        len = count - 1;
    memcpy(dest, src, len);
    dest[len] = 0;
    return res;
}

size_t s13_wcscpy(wchar_t * dest, const wchar_t * src, size_t size)
{
    size_t ret = s13_wcslen(src, size);
    size_t len = (ret >= size) ? size - 1 : ret;

    assert(dest);

    if (size) {
        wmemcpy(dest, src, len);
        dest[len] = L'\0';
    }
    return ret;
}

size_t s13_wcscat(wchar_t * dest, const wchar_t * src, size_t count)
{
    size_t dsize = s13_wcslen(dest, count);
    size_t len = s13_wcslen(src, count);
    size_t res = dsize + len;

    assert(dest);

    dest += dsize;
    count -= dsize;
    if (len >= count)
        len = count - 1;
    wmemcpy(dest, src, len);
    dest[len] = L'\0';
    return res;
}

//fnmatch
int s13_wildcmp(const char* dst, const char* src, const char any,
                const char one, const char escape){

    while(*src){
        //printf("%c & %c;", *src, *dst);
        if(*src == escape){
            src++;
            goto direct_check;
        }
        if(*src == any){
            while(*src == any) src++;//jump to first non-* character
            //bypass a dst character in the middle of the string
            //the above term is not needed, it just complicates things
            //now see if the *src is found in the rest of dst
            if(*src != one){
                dst = strchr(dst, *src);
                if(!dst) return -1;
            }
            //dst++;//this must point to the
            return s13_wildcmp(dst, src, any, one, escape);
        } else if(*src == one && *dst){
            while(*src == one && *dst){
                dst++;
                src++;
            }
            //return s13_wildcmp(dst, src, any, one);
        } else {
direct_check:
            if(*dst != *src) return -1;
            dst++;
            src++;
        }
    }
    if(strlen(dst) && *(src-1) != any) return -1;
    //printf("#\n");
    return 0;
}

error13_t s13_proglist_init(struct s13_proglist list, size_t chunk, char delim){
    list.buf = (char*)malloc(chunk?chunk:S13_PROGLIST_CHUNK_DEF);
    if(!list.buf) return e13_error(E13_NOMEM);
    list.chunk = chunk;
    list.delim = delim?delim:S13_PROGLIST_DELIM_DEF;
    list.pos = 0UL;
    list.bufsize = chunk;
    return E13_OK;
}

error13_t s13_proglist_add(struct s13_proglist list, char* string, size_t len){
    size_t slen = len?len:s13_strlen(string, S13_PROGLIST_STRLEN_MAX);
    if(list.pos + slen + 1 > list.bufsize){
        list.buf = realloc(list.buf, list.bufsize + list.chunk);
        if(!list.buf) return e13_error(E13_NOMEM);
    }
//    memcpy() TODO
	return e13_error(E13_IMPLEMENT);
}

#ifdef TEST
int test1_s13(){
    char str[100], delim[10], **ary, *prepared, **copy, *joined;
    char str1[100], str2[100], buf[200];
    char* pack[2];
    char escape;
    int n, i;
    printf("enter arg string: ");
    scanf("%s", str);
    printf("enter delimiter: ");
    scanf("%s", delim);

    //printf("prepared as \'%s\'\n", prepared = s13_prepare_exparts(str, delim));
    prepared = str;

    pack[0] = "\"";
    pack[1] = NULL;
    escape = '\\';

    n = s13_exparts(prepared, delim, pack, escape);
    printf("has %i arg parts\n", n);

    ary = s13_exmem(n);

    s13_explode(prepared, delim, pack, escape, ary);

    printf("exploded\n");
    for(i = 0; i < n; i++){
        printf("\t%i: %s\n", i, ary[i]);
    }

    printf("copied\n");
    copy = s13_copy_array(ary, n);
    if(!copy) printf("failed to copy, may crash after this!\n");
    else
        for(i = 0; i < n; i++){
            printf("\t%i: %s\n", i, copy[i]);
        }

    joined = s13_join_array(copy, delim, escape, n);

    printf("joined as: %s\n", joined);

    free(joined);

    s13_free_exmem(ary);

    printf("enter string1: ");
    scanf("%s", str1);

    printf("enter string2: ");
    scanf("%s", str2);

    s13_merge_exparts(str1, str2, pack, delim, escape, buf);

    printf("merged as \'%s\'\n", buf);

    return 0;
}
void test2_s13(){
    char src[100], dst[100];
    char* s;
    int i;

    char tdst[] = "12345";
    char* tsrc[] = {
                    "?12345", "12345?", "12?45", "123?45", "?12?345?", "?12345?",
                    "?????", "?", "????", "??????",
                    "*", "*12345*", "*123*45*", "123*45", "123**45", "12**5", "1*5",
                    "*?", "?*", "?12*5?", "*1?5*", "1*?5", "1*4?",
                    NULL
                    };

    for(i = 0; tsrc[i]; i++){
        printf("%s , %s -> %i\n", tdst, tsrc[i], s13_wildcmp(tdst, tsrc[i], '*', '?', '\\'));
    }

    printf("enter dst: ");
    gets(dst);
    printf("enter src: ");
    gets(src);
    printf("compare returns < %i >\n", s13_wildcmp(dst, src, '*', '?', '\\'));
}
int main(){
    test1_s13();
    //test2_s13();
    printf("press ctrl+c to end\n");
    while(0);
    return 0;
}
#endif
