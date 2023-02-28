#ifndef RR13_H
#define RR13_H

/*  Header for Request/Reply framework of lib13  */

#include "type13.h"
#include "error13.h"
#include "thread13.h"
#include "mem13.h"
#include "obj13.h"

#ifdef WIN32
#define RR13_NDEBUG //disable mutex_attr_np
#endif // WIN32

#define REQID13_ZERO    (0UL)
#define REQID13_INVAL   ((reqid13_t)-1)
#define REQID13_MAX   (REQID13_INVAL-1)

#define RR13_REQID_INVAL    REQID13_INVAL

#define RR13_REQID(a, i) ((a)*8*sizeof(uint32_t)+(i))

//#define RR13_REQ_CALLBACK   (0x01<<0)
//#define RR13_REQ_REPLY      (0x01<<1)
#define RR13_REQ_FREE       (0x01<<1) //free request after use
#define RR13_REQ_COPY       (0x01<<2) //make a copy of request
#define RR13_REQ_SHUTDOWN   (0x01<<6) //don't accept Req, exit ASANoMoreRequests
#define RR13_REQ_EXIT       (0x01<<7) //exit ASAP

struct rr13_request{

    int8_t flags;

    reqid13_t reqid;

    msegid13_t poolseg;

    void* buf;
    size_t bufsize;

    //process function
    error13_t (*proc)(void* arg, void* req, void* reply);
    void* proc_arg;

    //callback
    error13_t (*callback)(void* reply);
    void* context;//callback context

    struct rr13_request* next;

};

#define RR13_NEED_REPLY (0x01<<0)

#define RR13_REPLY_WAKEUP (0x01<<0)
#define RR13_REPLY_EXIT   (0x01<<1)

struct rr13_reply{

    int8_t flags;

    reqid13_t reqid;

    void* buf;
    size_t bufsize;

    msegid13_t poolseg;

    //callback
    error13_t (*callback)(void* reply);
    void* context;//callback context

    struct rr13_reply* next;
};

struct rr13_request_fifo{

    reqid13_t cur_req_id;
    uint32_t n;
    struct rr13_request* first, *last;

};

struct rr13_reply_fifo{

    reqid13_t cur_requid;
    uint32_t n;
    struct rr13_reply* first, *last, *cur;

};

#define RR13_NREQID_STP 1//default request id array members, step of growing

#define RR13_MEMPOOL    (0x01<<0)
#define RR13_REPLY      (0x01<<1)//start reply thread
#define RR13_NO_NEW_REQ (0x01<<2)//do not accept new requests

/*
 * these are request registeration flags, but WAIT flag also can be used
 * in the handle structure, so every process will start in WAIT state after
 * registeration
 */
#define RR13_PAUSE      (0x01<<3)//register the request, don't start it until
#define RR13_RESUME     (0x01<<4)//resume request process
#define RR13_SHUTTING_DOWN  (0x01<<5)//shutting down
#define RR13_EXIT       (0x01<<6)//exit

#define RR13_REPLY_BUCK    10

#define RR13_INIT_FLAGS (RR13_REPLY)

struct rr13{

    int8_t flags;
    magic13_t magic;

    /*
        the requests cannot have a mempool, they are dynamic
        char* request_mempool;
        size_t request_mempoolsize;
    */

    struct m13_mempool wait_requid_pool;
    struct m13_mempool reply_pool;
    struct m13_mempool request_pool;

    th13_t request_th;
    th13_t reply_th;

    th13_mutex_t request_mx;
    th13_mutex_t reply_mx;

    th13_sem_t request_sem;
    th13_sem_t reply_sem;

    th13_cond_t reply_cond;
    th13_cond_t standby_cond;

    reqid13_t* reqid_buf;
    reqid13_t nreqid_a;//number of array elements in the reqid_buf
    reqid13_t nfreereqid;

    struct rr13_request_fifo wait_list;
    struct rr13_request_fifo request_fifo;
    struct rr13_reply_fifo reply_fifo;

    struct e13 err;

};

#ifdef __cplusplus
    extern "C" {
#endif

error13_t rr13_init(struct rr13* h, int8_t flags);
error13_t rr13_destroy(struct rr13* h);

error13_t rr13_reg(struct rr13* h, struct rr13_request* req, objid13_t* req_id, int8_t flags);
error13_t rr13_checkout(struct rr13* h, objid13_t req_id, struct rr13_reply** reply);
error13_t rr13_wait(struct rr13 *h, reqid13_t req_id, struct rr13_reply **reply, int8_t flags);
error13_t rr13_cancel(struct rr13 *h, objid13_t req_id, struct rr13_request** req);

error13_t rr13_set_proc_flag(struct rr13* h, reqid13_t reqid, int8_t flags);

error13_t rr13_pause(struct rr13* h, reqid13_t reqid);
error13_t rr13_resume(struct rr13* h, reqid13_t reqid);

error13_t rr13_standby(struct rr13* h);
error13_t rr13_continue(struct rr13* h);

error13_t rr13_alloc_request(struct rr13* h, struct rr13_request** request);
error13_t rr13_free_request(struct rr13* h, struct rr13_request* request);
error13_t rr13_alloc_reply(struct rr13* h, struct rr13_reply** reply);
error13_t rr13_free_reply(struct rr13* h, struct rr13_reply* reply);

#ifdef __cplusplus
    }
#endif



#endif // RR13_H
