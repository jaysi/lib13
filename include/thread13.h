#ifndef THREAD13_H
#define THREAD13_H

#include "const13.h"

#ifndef TH13_SINGLE_THREAD

#include <semaphore.h>
#include <pthread.h>

typedef pthread_t th13_t;
typedef pthread_mutex_t th13_mutex_t;
typedef pthread_mutexattr_t th13_mutexattr_t;
typedef pthread_cond_t th13_cond_t;
typedef sem_t th13_sem_t;

#define th13_mutexattr_init     pthread_mutexattr_init
#define th13_mutexattr_destroy	pthread_mutexattr_destroy

#define TH13_MUTEX_INITIALIZER	PTHREAD_MUTEX_INITIALIZER

#define TH13_MUTEXATTR_INIT_ERRORCHECK(attrptr) MACRO(\
    th13_mutexattr_init(attrptr);\
    pthread_mutexattr_settype(attrptr, PTHREAD_MUTEX_ERRORCHECK);\
    )

#define th13_mutex_init(mxptr, mxattrptr)		pthread_mutex_init(mxptr, mxattrptr)==0?E13_OK:e13_error(E13_SYSE)
#define th13_mutex_destroy(mxptr)	pthread_mutex_destroy(mxptr)
#define th13_mutex_lock(mxptr)		pthread_mutex_lock(mxptr)
#define th13_mutex_unlock(mxptr)	pthread_mutex_unlock(mxptr)
#define th13_mutex_trylock(mxptr)	pthread_mutex_trylock(mxptr)

#define th13_sem_init       sem_init
#define th13_sem_destroy    sem_destroy
#define th13_sem_post       sem_post
#define th13_sem_wait       sem_wait

#define th13_cond_init      pthread_cond_init
#define th13_cond_destroy   pthread_cond_destroy
#define th13_cond_signal    pthread_cond_signal
#define th13_cond_wait      pthread_cond_wait

#define th13_create pthread_create
#define th13_detach pthread_detach
#define th13_join   pthread_join
#define th13_cancel pthread_cancel
#define th13_wait
#define th13_exit   pthread_exit

#else //TH13_SINGLE_THREAD

typedef int th13_t;
typedef int th13_mutex_t;
typedef int th13_mutexattr_t;
typedef int th13_cond_t;
typedef int th13_sem_t;

#define th13_mutexattr_init
#define th13_mutexattr_destroy

#define TH13_MUTEX_INITIALIZER true_

#define th13_mutex_init
#define th13_mutex_destroy
#define th13_mutex_lock
#define th13_mutex_unlock
#define th13_mutex_trylock

#define th13_sem_init
#define th13_sem_destroy
#define th13_sem_post
#define th13_sem_wait

#define th13_cond_init
#define th13_cond_destroy
#define th13_cond_signal
#define th13_cond_wait

#define th13_create
#define th13_detach
#define th13_join
#define th13_cancel
#define th13_wait

#endif//TH13_SINGLE_THREAD

struct th13_lock{
    int lock;
};

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef __cplusplus
    }
#endif

#endif // THREAD13_H
