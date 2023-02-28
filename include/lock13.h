#ifndef LOCK13_H
#define LOCK13_H

#include "type13.h"
#include "thread13.h"
#include "obj13.h"

#ifdef WIN32
#define LOCK13_NDEBUG //disable mutex_attr_np
#endif // WIN32

typedef uint8_t lock13_listhdr_flag_t;
typedef uint16_t lock13_t;
typedef uint8_t lock13_bitlockflag_t;

//memmory locks
#define LK13_RD (0x0001<<0) //read
#define LK13_WR (0x0001<<1) //write

#define _lock13_is_init(lock) ( (lock)->magic == MAGIC13_LK13?1:0 )
#define _lock13_is_list_init(list) MACRO( _lock13_is_bitlockinit(&((list)->hdr.lock)); )

#define LOCK13_BITLOCKF_NOREQ   (0x01<<0)//accept no more requests

struct lock13_bitlock{
    lock13_bitlockflag_t flags;
    magic13_t magic;
    size_t bitsize;
    bitmap13_t bmap;
    th13_mutex_t mx;
    th13_cond_t cond;
};

//#define LOCK13_LISTF_NOLK (0x01<<0)
//#define LOCK13_LISTF_DEF  (LOCK13_LIST_FLAG_NOLK)

//struct lock13_list_hdr{
//    lock13_listhdr_flag_t flags;
//    objid13_t n;
//    objid13_t ref;//object refrence counter
//    struct lock13_bitlock lock;
//};

error13_t lock13_init_bitlock(struct lock13_bitlock* l13, size_t nbits);
error13_t lock13_destroy_bitlock(struct lock13_bitlock *l13);

error13_t lock13_lockbit(struct lock13_bitlock* l13, size_t bitno);
error13_t lock13_unlockbit(struct lock13_bitlock* l13, size_t bitno);

//the lock argument actually shows what you want to do with object, wanna read?
//(search_fn)(void* list, objid13_t objid, struct obj13** obj),
//returns E13_OK on find
//obj13** obj

#define _lock13_lock_mx(list) MACRO( th13_mutex_lock(&((list)->hdr.lock.mx))?e13_error(E13_SYSE):E13_OK; )
#define _lock13_unlock_mx(list) MACRO( th13_mutex_unlock(&((list)->hdr.lock.mx))?e13_error(E13_SYSE):E13_OK; )

//this is bullshit! don't use it
/*
#define lock13_lockobj(list, objid, type, search_fn, obj)\
        MACRO (\
            assert(obj);\
            if(!_lock13_is_list_init(list)){\
                e13_error(E13_MISUSE);\
            } else {\
                if(search_fn(list, objid, obj) != E13_OK){\
                    _lock13_unlock_mx(list);\
                    e13_error(E13_NOTFOUND);\
                } else {\
                    if((*obj)->flags & OBJ13_FLAG_NOREQ){\
                        _lock13_unlock_mx(list);\
                        e13_error(E13_LOCKED);\
                    else {\
                        while( (*obj)->flags & OBJ13_FLAG_WR || (type & LK13_WR && (*obj)->rd) ){\
                            _lock13_unlock_mx(list);\
                            th13_cond_wait(&((list)->hdr.lock.cond), &((list)->hdr.lock.mx));\
                        }\
                        if(type & LK13_RD) (*obj)->rd++;\
                        if(type & LK13_WR) (*obj)->flags |= OBJ13_FLAG_WR;\
                        _lock13_unlock_mx(list);\
                    }\
                }\
            }\
            E13_OK;\
        )
*/
error13_t lock13_unlockobj( void* list,
                            objid13_t objid,
                            lock13_t type,
                            error13_t (search_fn)(void* list, objid13_t objid, struct obj13** obj)//returns E13_OK on find
                            );

#endif // LOCK13_H
