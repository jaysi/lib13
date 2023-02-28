#include "include/lock13.h"
#include "include/const13.h"
#include "include/mem13.h"

error13_t lock13_init_bitlock(struct lock13_bitlock* l13, size_t nbits){

#ifndef LOCK13_NDEBUG
    th13_mutexattr_t mxattr;
#endif
    if(_lock13_is_init(l13)) return e13_error(E13_MISUSE);
    l13->bitsize = nbits;
    l13->bmap = (bitmap13_t)m13_malloc(BITMAPSIZE(nbits)*BITMAPELEMENTSIZE);
    if(!l13->bmap) return e13_error(E13_NOMEM);

// mutex init
#ifndef LOCK13_NDEBUG
    th13_mutexattr_init(&mxattr);
    pthread_mutexattr_setkind_np(&mxattr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
    if(!th13_mutex_init(&(l13->mx),
#ifndef LOCK13_NDEBUG
    &mxattr
#else
    NULL
#endif
    )){
        m13_free(l13->bmap);
#ifndef LOCK13_NDEBUG
    th13_mutexattr_destroy(&mxattr);
#endif
        return e13_error(E13_SYSE);
    }

#ifndef LOCK13_NDEBUG
    th13_mutexattr_destroy(&mxattr);
#endif
//end of mutex init

//cond init
    if(!th13_cond_init(&(l13->cond), NULL)){
        th13_mutex_destroy(&(l13->mx));
        m13_free(l13->bmap);
        return e13_error(E13_SYSE);
    }
//end of cond init

    l13->flags = 0;
    l13->magic = MAGIC13_LK13;
    return E13_OK;
}

error13_t lock13_destroy_bitlock(struct lock13_bitlock* l13){

    if(!_lock13_is_init(l13)) return e13_error(E13_MISUSE);

    th13_mutex_destroy(&(l13->mx));
    th13_cond_destroy(&(l13->cond));
    m13_free(l13->bmap);

    l13->magic = MAGIC13_INV;

    return E13_OK;
}

error13_t lock13_lockbit(struct lock13_bitlock *l13, size_t bitno){
    if(!_lock13_is_init(l13)) return e13_error(E13_MISUSE);

    if(l13->flags & LOCK13_BITLOCKF_NOREQ) return e13_error(E13_ABORT);

#ifndef LOCK13_NDEBUG
    assert(bitno < l13->bitsize);
#endif

    th13_mutex_lock(&(l13->mx));

    while(ISBITMAP(l13->bmap, bitno))
        th13_cond_wait(&(l13->cond), &(l13->mx));

    BITMAPON(l13->bmap, bitno);

    th13_mutex_unlock(&(l13->mx));

    return E13_OK;
}

error13_t lock13_unlockbit(struct lock13_bitlock *l13, size_t bitno){
    if(!_lock13_is_init(l13)) return e13_error(E13_MISUSE);

    if(l13->flags & LOCK13_BITLOCKF_NOREQ) return e13_error(E13_ABORT);

    assert(bitno < l13->bitsize);

    th13_mutex_lock(&(l13->mx));

#ifndef LOCK13_NDEBUG
    assert(ISBITMAP(l13->bmap, bitno));
#endif
    BITMAPOFF(l13->bmap, bitno);

    //TODO: check sequence
    th13_mutex_unlock(&(l13->mx));
    th13_cond_signal(&(l13->cond));

    return E13_OK;

}
