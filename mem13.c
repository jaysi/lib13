/* Copyright (C) 1991,92,93,94,96,97,98,2000,2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define MEM13_NDEBUG

#include "include/debug13.h"
#include "include/mem13.h"
#include "include/error13.h"
#include "include/bit13.h"

#ifndef _LIBC
# define __builtin_expect(expr, val)   (expr)
#endif

#undef memmem

#define _lock() MACRO( if(!(pool->flags & MEM13_NOLOCK)) th13_mutex_lock(&pool->mx); )
#define _unlock() MACRO( if(!(pool->flags & MEM13_NOLOCK)) th13_mutex_unlock(&pool->mx); )
#define _wait_nlocks() MACRO( while(pool->nlocks){if(!(pool->flags & MEM13_NOLOCK)) th13_cond_wait(&pool->cond, &pool->mx);} )
#define _wait_bitlock() MACRO( while(ISBITMAP((pool)->lockbitmap, seg)){if(!(pool->flags & MEM13_NOLOCK)) th13_cond_wait(&pool->cond, &pool->mx);} )
#define _deb_m2d _NullMsg
//#define MTEST //turn on malloc test

/* Return the first occurrence of NEEDLE in HAYSTACK. */
void *memmem (const void *haystack, size_t haystack_len, const void *needle, size_t needle_len){
  /* not really Rabin-Karp, just using additive hashing */
  char* haystack_ = (char*)haystack;
  char* needle_ = (char*)needle;
  int hash = 0;		/* this is the static hash value of the needle */
  int hay_hash = 0;	/* rolling hash over the haystack */
  char* last;
  size_t i;

  if (haystack_len < needle_len)
    return NULL;

  if (!needle_len)
    return haystack_;

  /* initialize hashes */
  for (i = needle_len; i; --i)
    {
      hash += *needle_++;
      hay_hash += *haystack_++;
    }

  /* iterate over the haystack */
  haystack_ = (char*)haystack;
  needle_ = (char*)needle;
  last = haystack_+(haystack_len - needle_len + 1);
  for (; haystack_ < last; ++haystack_)
    {
      if (__builtin_expect(hash == hay_hash, 0) &&
      *haystack_ == *needle_ &&	/* prevent calling memcmp, was a optimization from existing glibc */
      !memcmp (haystack_, needle_, needle_len))
    return haystack_;

      /* roll the hash */
      hay_hash -= *haystack_;
      hay_hash += *(haystack_+needle_len);
    }

  return NULL;
}

error13_t m13_pool_init(struct m13_mempool* mempool,
                        size_t segsize, msegid13_t nseg,
                        size_t buck,
                        int8_t policy, int8_t flags){

    error13_t ret;
    size_t bitmapsize;
    th13_mutexattr_t mxattr;

    if(_m13_pool_is_init(mempool)) return e13_error(E13_MISUSE);

    if((ret = e13_init(&mempool->err, E13_MAX_WARN_DEF, E13_MAX_ESTR_DEF, LIB13_MEM)) != E13_OK){
        return ret;
    }

    mempool->segsize = segsize;
    mempool->nseg = nseg;
    mempool->buck = buck;
    mempool->nlocks = 0;

    bitmapsize = BITMAPSIZE(nseg);

    mempool->bitmap = (bitmap13_t)m13_malloc(bitmapsize);
    mempool->lockbitmap = (bitmap13_t)m13_malloc(bitmapsize);

    if(!mempool->bitmap || !mempool->lockbitmap){
        return e13_ierror(&mempool->err, E13_NOMEM, "s", "while trying to malloc for bitmap of mempool");
    }

    mempool->buf = (char*)malloc(nseg*segsize);

    if(!mempool->buf){
        free(mempool->bitmap);
        free(mempool->lockbitmap);
        return e13_ierror(&mempool->err, E13_NOMEM, "s", "while trying to malloc for buffer of mempool");
    }

    memset(mempool->bitmap, 0, bitmapsize);
    memset(mempool->lockbitmap, 0, bitmapsize);

    mempool->nfree = nseg;

    mempool->policy = policy;
    mempool->flags = flags;

    if(!(flags & MEM13_NOLOCK)){

    // mutex init
    #ifndef MEM13_NDEBUG
        th13_mutexattr_init(&mxattr);
        pthread_mutexattr_setkind_np(&mxattr, PTHREAD_MUTEX_ERRORCHECK_NP);
    #endif
        if(!th13_mutex_init(&(mempool->mx),
    #ifndef MEM13_NDEBUG
        &mxattr
    #else
        NULL
    #endif
        )){
            free(mempool->bitmap);
            free(mempool->lockbitmap);
        #ifndef MEM13_NDEBUG
            th13_mutexattr_destroy(&mxattr);
        #endif
            return e13_error(E13_SYSE);
        }

    #ifndef MEM13_NDEBUG
        th13_mutexattr_destroy(&mxattr);
    #endif
    //end of mutex init

    //cond init
        if(!th13_cond_init(&(mempool->cond), NULL)){
            th13_mutex_destroy(&(mempool->mx));
            free(mempool->bitmap);
            free(mempool->lockbitmap);
            return e13_error(E13_SYSE);
        }
    //end of cond init
    }

    mempool->magic = MAGIC13_M13;

    return E13_OK;
}

void* m13_pool_alloc(struct m13_mempool* pool, msegid13_t* seg){

    msegid13_t i;
    //bitmap13_t bitmap;

    _lock();

    if(!_m13_pool_is_init(pool)){
        _unlock();
//        return e13_error(E13_MISUSE);
        e13_uerror(&pool->err, E13_MISUSE, "s", "not init");
        return NULL;
    }
    e13_cleanup(&pool->err);

    if( !pool->nfree &&
        ((pool->policy == MEM13_EXPANDING) ||
        (pool->policy == MEM13_AUTOSIZE))
        ){

        _wait_nlocks();

        if(_m13_pool_chsize(pool, 0) == E13_OK){

            return m13_pool_alloc(pool, seg);

        }

    }

    for(i = 0; i < pool->nseg; i++){
        if(!ISBITMAP(pool->bitmap, i)){
            BITMAPON(pool->bitmap, i);
            pool->nfree--;
            *seg = i;

            pool->nlocks++;
            BITMAPON(pool->lockbitmap, i);

            //_unlock();
            return BITMAPBUF(pool->buf, pool->segsize, i);
            //return E13_OK;
        }
    }

    _unlock();

    e13_ierror(&pool->err, E13_ERROR, "s", "unknown case while allocating pool memmory");
    return NULL;
}

error13_t m13_pool_free(struct m13_mempool* pool, msegid13_t seg){

    _lock();

    if(!_m13_pool_is_init(pool)){
        _unlock();
        return e13_error(E13_MISUSE);
    }
    e13_cleanup(&pool->err);

    _wait_bitlock();

    BITMAPOFF((pool)->bitmap, seg);
    pool->nfree++;

    if( pool->nfree >= pool->buck &&
        pool->policy == MEM13_AUTOSIZE){
    }

    _unlock();

    return E13_OK;

}

error13_t _m13_pool_chsize(struct m13_mempool *pool, int shrink){

    size_t oldbitmapsize, newbitmapsize;

    if(!_m13_pool_is_init(pool)) return e13_error(E13_MISUSE);

    e13_cleanup(&pool->err);

    oldbitmapsize = BITMAPSIZE(pool->nseg);

    if(shrink){
        pool->nseg -= pool->buck;
    } else {
        pool->nseg += pool->buck;
    }

    newbitmapsize = BITMAPSIZE(pool->nseg);

    if(newbitmapsize != oldbitmapsize ){

        pool->bitmap = (bitmap13_t)realloc(pool->bitmap, newbitmapsize);
        pool->lockbitmap = (bitmap13_t)realloc(pool->lockbitmap, newbitmapsize);

        if(newbitmapsize > oldbitmapsize){
            memset(pool->bitmap + (newbitmapsize - oldbitmapsize), 0, (newbitmapsize - oldbitmapsize));
            memset(pool->lockbitmap + (newbitmapsize - oldbitmapsize), 0, (newbitmapsize - oldbitmapsize));
        }

        if(!pool->bitmap || !pool->lockbitmap){
            return e13_ierror(&pool->err, E13_NOMEM, "s", "while trying to realloc for bitmap of mempool");
        }

    }

    pool->buf = realloc(pool->buf, pool->nseg*pool->segsize);
    if(!pool->buf){
        free(pool->bitmap);
        free(pool->lockbitmap);
        return e13_ierror(&pool->err, E13_NOMEM, "s", "while trying to realloc for buffer of mempool");
    }

    return E13_OK;
}

error13_t m13_pool_destroy(struct m13_mempool* pool){

    _lock();

    if(!_m13_pool_is_init(pool)){
        _unlock();
        return e13_error(E13_MISUSE);
    }
    e13_cleanup(&pool->err);

    _wait_nlocks();

    if(pool->bitmap) free(pool->bitmap);
    if(pool->lockbitmap) free(pool->lockbitmap);
    if(pool->buf) free(pool->buf);
    pool->magic = MAGIC13_INV;

    _unlock();

    if(!(pool->flags & MEM13_NOLOCK)){
        th13_mutex_destroy(&pool->mx);
        th13_cond_destroy(&pool->cond);
    }

    return E13_OK;
}

msegid13_t m13_pool_find(struct m13_mempool* pool, void* buf){

    msegid13_t seg;

    _lock();

    if(!_m13_pool_is_init(pool)){
        _unlock();
        return e13_error(E13_MISUSE);
    }
    e13_cleanup(&pool->err);

    for(seg = 0; seg < pool->nseg; seg++){

        _wait_bitlock();

        if(ISBITMAP(pool->bitmap, seg)){
            if(!memcmp(_m13_pool_buf(pool, seg), buf, pool->segsize)){
                BITMAPON(pool->lockbitmap, seg);
                pool->nlocks++;
                _unlock();
                return seg;
            }

        }
    }

    _unlock();

    e13_ierror(&pool->err, E13_NOTFOUND, "s","the requested contents was not"
                                             "found inside the buffer");
    return MSEGID13_INVAL;

}

//error13_t m13_pool_set(struct m13_mempool* pool, msegid13_t seg, void* val){

//    if(!_m13_pool_is_init(pool)) return e13_error(E13_MISUSE);
//    if(seg > pool->nseg) return e13_ierror(&pool->err, E13_RANGE, "s",
//                                "segment value out of range");

//    memcpy(_m13_pool_buf(pool, seg), val, pool->segsize);
//    return E13_OK;

//}

msegid13_t m13_pool_upper(struct m13_mempool *pool){

    msegid13_t upper;

    _lock();

    if(!_m13_pool_is_init(pool)){
        _unlock();
        return e13_error(E13_MISUSE);
    }
    e13_cleanup(&pool->err);

    upper = pool->nseg;

    _unlock();

    return upper;

}

error13_t m13_pool_unlock(struct m13_mempool *pool, msegid13_t seg){

    _lock();

    if(!_m13_pool_is_init(pool)){
        _unlock();
        return e13_error(E13_MISUSE);
    }
    e13_cleanup(&pool->err);

    BITMAPOFF(pool->lockbitmap, seg);
    pool->nlocks--;

    _unlock();
    if(!(pool->flags & MEM13_NOLOCK)) th13_cond_signal(&pool->cond);

    return E13_OK;

}

void* m13_pool_buf(struct m13_mempool *pool, msegid13_t seg){

    _lock();

    if(!_m13_pool_is_init(pool)){
        _unlock();
        e13_uerror(&pool->err, E13_MISUSE, "s", "not init");
        return NULL;
//        return e13_error(E13_MISUSE);
    }
    e13_cleanup(&pool->err);

    _wait_bitlock();

    if(!ISBITMAP(pool->bitmap, seg)){
        _unlock();
        return NULL;
    }

    BITMAPON(pool->lockbitmap, seg);
    pool->nlocks++;

    _unlock();

    return _m13_pool_buf(pool, seg);

}

/*
 *2d array memmory allocator,
 *total = 0 -> calculate needed memmory
 *same size = 1 -> all array elements are same size
*/
void** m13_malloc_2d(size_t n, size_t total, int same_size, ...){
    va_list va;
    size_t i;
    void** ptr;

    size_t total_size;
    size_t seg_size;

    if(same_size){
    	_deb_m2d("same size");
        va_start(va, same_size);

        seg_size = va_arg(va, size_t);

        va_end(va);

        _deb_m2d("segment size %i", seg_size);
    }

    if(!total){//means that calc it yourself!

		_deb_m2d("total = 0, calc it yourself");

        if(same_size){//same size?

            total_size = n*seg_size;
            _deb_m2d("total_size = %i*%i = %u", n , seg_size, total_size);

        } else {//not same size

            total_size = 0UL;

            va_start(va, same_size);

            for(i = 0; i < n; i++){
                total_size += va_arg(va, size_t);
            }

            _deb_m2d("total_size = %u", total_size);

            va_end(va);

        }

        total_size += n*sizeof(void**);

        _deb_m2d("total_size += n*sizeof(void**) = %u", total_size);

    } else {

        total_size = n*sizeof(void**) + total;

        _deb_m2d("total_size += n*sizeof(void**) + total(%u) = %u", total, total_size);

    }

    ptr = (void**)m13_malloc(total_size);
    if(!ptr) return NULL;

    //use total size as pos, start to save data after the last pointer
    total_size = n*sizeof(void**);

    if(same_size){

        for(i = 0; i < n; i++){
            ptr[i] = ptr + total_size;
            total_size += seg_size;
        }

    } else {

        va_start(va, same_size);

        for(i = 0; i < n; i++){
            ptr[i] = ptr + total_size;
            total_size += va_arg(va, size_t);
        }

        va_end(va);
    }

    return ptr;
}

void* m13_realloc(void* ptr, size_t size){
    if((size&&!ptr) || (!size&&ptr)){
        return realloc(ptr, size);
    }
    return NULL;
}
void* m13_malloc(size_t size){
    if(size) return malloc(size);
    return NULL;
}

void m13_free(void* ptr){
    if(ptr) free(ptr);
}


