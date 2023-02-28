#ifndef MEM13_H
#define MEM13_H

#include <string.h>

#include "type13.h"
#include "error13.h"
#include "const13.h"
#include "lock13.h"

#ifdef WIN32
#define MEM13_NDEBUG //disable mutex_attr_np
#endif // WIN32

#define MEM13_STATIC    0
#define MEM13_DYNAMIC   1

#define MEM13_EXPANDING         (0x1)
#define MEM13_AUTOSIZE          (0x2)

#define MEM13_NOLOCK            (0x1<<0)

#define MEM13_DEF_POLICY    MEM13_EXPANDING
#define MEM13_DEF_FLAGS     (0x0)
#define MEM13_DEF_BUCK      10//10 segments each time!

#define MSEGID13_INVAL  ((msegid13_t)-1)

/*
 * the mempool is designed to work with fixed-size element dynamic arrays
*/

struct m13_mempool{
    magic13_t magic;
    int8_t policy:4, flags:4;

    size_t buck;//bucket size
    size_t segsize;
    msegid13_t nseg;
    msegid13_t nfree;

    struct e13 err;

    th13_mutex_t mx;
    th13_cond_t cond;

    bitmap13_t bitmap;//1 for allocated, 0 for free
    msegid13_t nlocks;
    bitmap13_t lockbitmap;//1 for segment locked
    void* buf;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifndef memmem
void *memmem (	const void *haystack, size_t haystack_len,
                const void *needle, size_t needle_len);
#endif

/*memmory pool*/
#define _m13_pool_is_init(pool) ((pool)->magic==MAGIC13_M13?1:0)
error13_t m13_pool_init(struct m13_mempool* mempool,
                        size_t segsize, msegid13_t nseg,
                        size_t buck,
                        int8_t policy, int8_t flags);
error13_t m13_pool_destroy(struct m13_mempool* pool);
void* m13_pool_alloc(struct m13_mempool* pool, msegid13_t* seg);
void* m13_pool_buf(struct m13_mempool* pool, msegid13_t seg);
error13_t m13_pool_free(struct m13_mempool* pool, msegid13_t seg);
error13_t _m13_pool_chsize(struct m13_mempool *pool, int shrink);
msegid13_t m13_pool_find(struct m13_mempool* pool, void* buf);
//error13_t m13_pool_set(struct m13_mempool* pool, msegid13_t seg, void* val);
error13_t m13_pool_unlock(struct m13_mempool* pool, msegid13_t seg);
msegid13_t m13_pool_upper(struct m13_mempool* pool);
void* m13_realloc(void* ptr, size_t size);
void* m13_malloc(size_t size);
void m13_free(void* ptr);

//#define m13_pool_expand(pool) _m13_pool_chsize(pool, 0)
//#define m13_pool_shrink(pool) _m13_pool_chsize(pool, 1)

#define _m13_pool_buf(pool, seg) ((pool)->buf+(pool)->segsize*(seg))

void** m13_malloc_2d(size_t n, size_t total, int same_size, ...);//... = size list
#define m13_free_2d m13_free

#ifdef __cplusplus
}
#endif


#endif // MEM13_H
