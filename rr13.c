/* This is Request/Reply framework used in the lib13 */

#include "include/rr13.h"

#define _rr13_is_init(h) ((h)->magic==MAGIC13_RR13)?1:0

struct rr13_request* _rr13_wait_request(struct rr13* h){

    struct rr13_request* req;

    th13_sem_wait(&h->request_sem);
    th13_mutex_lock(&h->request_mx);

    req = h->request_fifo.first;
    if(req){
        h->request_fifo.first = h->request_fifo.first->next;
        h->request_fifo.n--;
        h->request_fifo.cur_req_id = req->reqid;
    }

    if((!h->request_fifo.n) && (h->flags & RR13_SHUTTING_DOWN)){
        h->flags |= RR13_EXIT;
    }

    th13_mutex_unlock(&h->request_mx);

    return req;
}

struct rr13_reply* _rr13_wait_reply(struct rr13* h){

    struct rr13_reply* rep;

    th13_sem_wait(&h->reply_sem);
    th13_mutex_lock(&h->reply_mx);

    rep = h->reply_fifo.first;
    if(rep){
        h->reply_fifo.first = h->reply_fifo.first->next;
        h->reply_fifo.n--;
    }

    th13_mutex_unlock(&h->reply_mx);

    return rep;
}

error13_t _rr13_reg_reply(struct rr13 *h, struct rr13_reply* reply){

    reply->next = NULL;

    th13_mutex_lock(&h->reply_mx);

    if(!h->reply_fifo.first){

        h->reply_fifo.first = reply;
        h->reply_fifo.last = reply;
        h->reply_fifo.n = 1UL;

    } else {

        h->reply_fifo.last->next = reply;
        h->reply_fifo.last = reply;
        h->reply_fifo.n++;

    }

    th13_sem_post(&h->reply_sem);
    th13_mutex_unlock(&h->reply_mx);

    return E13_OK;
}

void* _rr13_request_thread(void* arg){

    struct rr13* h = (struct rr13*)arg;
    struct rr13_request* req;
    struct rr13_reply* reply;
    size_t poolseg;

    for(;;){

        if(h->flags & RR13_MEMPOOL){
            reply = (struct rr13_reply*)m13_pool_alloc(&h->reply_pool, &poolseg);
            if(reply) reply->poolseg = poolseg;
        } else {
            reply = (struct rr13_reply*)m13_malloc(sizeof(struct rr13_reply));
        }

        req = _rr13_wait_request(h);

        if(req){

            //exit thread, exit reply thread too!
            if((req->flags & RR13_REQ_EXIT) || (h->flags & RR13_EXIT)){

                if(reply){
                    reply->flags |= RR13_REPLY_EXIT;
                    _rr13_reg_reply(h, reply);
                } else {
                    th13_cancel(h->reply_th);
                }

                break;

            }

            //wait for all tasks to finish, exit
            if(req->flags & RR13_REQ_SHUTDOWN){

                h->flags |= RR13_NO_NEW_REQ;
                h->flags |= RR13_SHUTTING_DOWN;

                break;

            }

            if(req->proc){
                req->proc(req->proc_arg, (void*)req, (void*)reply);
            }

            if(reply){

                reply->callback = req->callback;
                reply->context = req->context;

                reply->reqid = req->reqid;
                _rr13_reg_reply(h, reply);
            }

            if((req->flags & RR13_REQ_FREE) || (req->flags & RR13_REQ_COPY)){

                if(h->flags & RR13_MEMPOOL){
                    m13_pool_free(&h->request_pool, req->poolseg);
                } else {
                    m13_free(req);
                }

            }

        } else {//ignore, this could happen when pausing/resuming

            //avoid leak
            if(h->flags & RR13_MEMPOOL){
                if(reply) m13_pool_free(&h->reply_pool, reply->poolseg);
            } else {
                if(reply) m13_free(reply);
            }

        }
    }

    return NULL;
}

error13_t _rr13_trigger_reply_waiter(struct rr13* h, struct rr13_reply* reply){

    error13_t ret = E13_OK;
    msegid13_t seg;
    struct rr13_reply* prev;

    th13_mutex_lock(&h->reply_mx);

    //look into the wait list, if the reqid is expected, trigger cond.
    if( (seg = m13_pool_find(&h->wait_requid_pool, (void*)&reply->reqid))
            != MSEGID13_INVAL){

        h->reply_fifo.cur_requid = reply->reqid;

        if( !(reply->flags & RR13_REPLY_WAKEUP) ){

            h->reply_fifo.cur = reply;

        } else {

            if(h->flags & RR13_MEMPOOL){
                m13_pool_free(&h->reply_pool, reply->poolseg);
            } else {
                free(reply);
            }

            for(h->reply_fifo.cur = h->reply_fifo.first; h->reply_fifo.cur;
                h->reply_fifo.cur = h->reply_fifo.cur->next){

                if(h->reply_fifo.cur->reqid == reply->reqid){
                    break;
                } else {
                    prev = h->reply_fifo.cur;
                }

            }

        }

        m13_pool_free(&h->wait_requid_pool, seg);

        th13_cond_signal(&h->reply_cond);

    }

    th13_mutex_unlock(&h->reply_mx);

    return ret;

}

void* _rr13_reply_thread(void* arg){

    struct rr13* h = (struct rr13*)arg;
    struct rr13_reply* reply;

    for(;;){

        reply = _rr13_wait_reply(h);

        if(reply){

            if(reply->flags & RR13_REPLY_EXIT){
                //exit thread NOW!
                break;
            }

            if( !(reply->flags & RR13_REPLY_WAKEUP) ){

                //be carefull using the reply pointer as it is being re-used

                if(reply->callback){//the proc function may set this

                    reply->callback((void*)reply);

                }

            }

            if(reply->flags & RR13_NEED_REPLY){//the proc function may set this

                _rr13_trigger_reply_waiter(h, reply);

            }

        } else {//exit thread

            break;

        }

    }

    return NULL;

}

error13_t rr13_init(struct rr13* h, int8_t flags){

    error13_t ret;

    th13_mutexattr_t attr;

    if(_rr13_is_init(h)){
        return e13_ierror(&h->err, E13_MISUSE, "s", "rr13 handle already"
                                                    "initialized");
    }

    if((ret = e13_init(&h->err, E13_MAX_WARN_DEF, E13_MAX_ESTR_DEF, LIB13_RR)) != E13_OK){
        return ret;
    }

    if(flags & RR13_REPLY){

        if(m13_pool_init(   &h->wait_requid_pool, sizeof(reqid13_t), RR13_REPLY_BUCK,
                            RR13_REPLY_BUCK, MEM13_AUTOSIZE, MEM13_DEF_FLAGS) !=
                             E13_OK
                            ){
            e13_copy(&h->err, &(h->wait_requid_pool.err));
            e13_destroy(&(h->wait_requid_pool.err));

            return e13_errcode(&h->err);
        }

        if(flags & RR13_MEMPOOL){

            if(m13_pool_init(   &h->reply_pool, sizeof(struct rr13_reply), RR13_REPLY_BUCK,
                                RR13_REPLY_BUCK, MEM13_AUTOSIZE, MEM13_DEF_FLAGS) !=
                                 E13_OK
                                ){

                e13_copy(&h->err, &(h->reply_pool.err));
                e13_destroy(&(h->reply_pool.err));

                return e13_errcode(&h->err);
            }

        }
    }

    h->flags = flags;
    h->request_fifo.first = NULL;
    h->wait_list.first = NULL;
    h->reply_fifo.first = NULL;
    //h->reply_fifo.new_req_id = REQID13_INVAL;
    h->request_fifo.cur_req_id = REQID13_INVAL;

    h->nreqid_a = 0UL;
    h->reqid_buf = NULL;
    h->nfreereqid = 0UL;


#ifndef RR13_NDEBUG
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP); //_np variant deprecated now
#else
    pthread_mutexattr_init(&attr);
    //pthread_mutexattr_setkind(&attr, PTHREAD_MUTEX);
#endif

    th13_mutex_init(&h->request_mx, &attr);
    th13_sem_init(&h->request_sem, 0, 0);
    if(flags & RR13_REPLY){
        th13_mutex_init(&h->reply_mx, &attr);
        th13_sem_init(&h->reply_sem, 0, 0);
        th13_cond_init(&h->reply_cond, NULL);
    }

    th13_mutexattr_destroy(&attr);

    if(th13_create(&(h->request_th), NULL, &_rr13_request_thread,
                            (void*)h) < 0){

        th13_mutex_destroy(&h->request_mx);
        th13_sem_destroy(&h->request_sem);

        if(flags && RR13_REPLY){

            th13_mutex_destroy(&h->reply_mx);
            th13_sem_destroy(&h->reply_sem);
            th13_cond_destroy(&h->reply_cond);

        }

        return e13_ierror(&h->err, E13_SYSE, "s", "creating request thread");

    }

    th13_detach(h->request_th);

    if(flags & RR13_REPLY){
        if(th13_create(&(h->reply_th), NULL, &_rr13_reply_thread,
                                (void*)h) < 0){

            th13_mutex_destroy(&h->request_mx);
            th13_sem_destroy(&h->request_sem);

            th13_mutex_destroy(&h->reply_mx);
            th13_sem_destroy(&h->reply_sem);
            th13_cond_destroy(&h->reply_cond);

            th13_cancel(h->request_th);

            m13_pool_destroy(&h->wait_requid_pool);

            if(flags && RR13_MEMPOOL){
                m13_pool_destroy(&h->reply_pool);
            }

            return e13_ierror(&h->err, E13_SYSE, "s", "creating reply thread");

        }

        th13_detach(h->reply_th);
    }

    h->magic = MAGIC13_RR13;

    return E13_OK;
}

error13_t rr13_destroy(struct rr13 *h){

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    e13_cleanup(&h->err);

    //1. don't accept new requests
    //2. submit exit request to REQUEST thread
    //3. wait for thread to exit
    //3a.destroy all REQUEST elements
    //4. submit exit request to REPLY thread
    //5. wait for thread to exit
    //3a.destroy all REPLY elements

    e13_destroy(&(h->wait_requid_pool.err));

    th13_mutex_lock(&h->request_mx);
    h->flags |= RR13_NO_NEW_REQ;
    th13_mutex_unlock(&h->request_mx);

    //exit request thread
    th13_join(h->request_th, NULL);

    //rr13_reg(h, NULL, NULL);

    th13_wait(h->request_th);

    if(h->flags & RR13_REPLY){

        th13_join(h->reply_th, NULL);
        _rr13_reg_reply(h, NULL);
        th13_wait(h->reply_th);

        th13_mutex_destroy(&h->request_mx);
        th13_sem_destroy(&h->request_sem);

        th13_mutex_destroy(&h->reply_mx);
        th13_sem_destroy(&h->reply_sem);
        th13_cond_destroy(&h->reply_cond);

        m13_pool_destroy(&h->wait_requid_pool);

        if(h->flags && RR13_MEMPOOL){
            m13_pool_destroy(&h->reply_pool);
        }

    }

    return E13_OK;
}

static inline reqid13_t _rr13_get_free_reqid(struct rr13 *h){

    size_t a;
    size_t i;
    size_t start = 0UL;

    e13_cleanup(&h->err);

    //no locking required

    if(!h->nfreereqid){

        h->reqid_buf = (reqid13_t*)m13_realloc(  h->reqid_buf,
                                                (start + RR13_NREQID_STP)*
                                                sizeof(reqid13_t)
                                                );

        if(!h->reqid_buf){
            e13_ierror(&h->err, E13_NOMEM, "s", "while allocating for "
                        "request_id_buffer");
            return RR13_REQID_INVAL;
        }

        start = h->nreqid_a;
        h->reqid_buf[h->nreqid_a] = REQID13_ZERO;

        h->nfreereqid = RR13_NREQID_STP*8*sizeof(reqid13_t);
        h->nreqid_a += RR13_NREQID_STP;
    }

    for(a = start; a < h->nreqid_a; a++){
        for(i = 0; i <= sizeof(reqid13_t)*8-1; i++){
            /*TODO: THESE +a s might become annoying*/
            if( !ISBIT(h->reqid_buf+a, i) ){
                BITON(h->reqid_buf+a, i);
                h->nfreereqid--;
                return RR13_REQID(a, i);
            }
        }
    }

    return RR13_REQID_INVAL;
}

static inline error13_t _rr13_set_free_reqid(struct rr13 *h, reqid13_t reqid){

    size_t a;
    int i;

    a = reqid % (sizeof(reqid13_t));
    i = (reqid / (sizeof(reqid13_t)*8));

    //TODO: take care of +a
    BITOFF(h->reqid_buf+a, ((reqid13_t)1)<<i);

    return E13_OK;

}

error13_t rr13_wait(struct rr13 *h, reqid13_t req_id, struct rr13_reply **reply, int8_t flags){

    /*
     *  TODO: WHAT IF THE THREAD PROCESSES THE REQUEST SO FAST, BEFORE CALLING
     *  THIS FUNCTION? the reply->reqid will be replaced by the next proc
     *  before being extracted using this.
    */

    error13_t ret;
    reqid13_t wait_requid_seg;
    reqid13_t requidptr = req_id;
    reqid13_t* rbuf;

    if(!_rr13_is_init(h)){

        return e13_error(E13_MISUSE);

    }

    ret = e13_cleanup(&h->err);

    th13_mutex_lock(&h->reply_mx);

    //register the reply_wait in the list
    m13_pool_alloc(&h->wait_requid_pool, &wait_requid_seg);
    rbuf = m13_pool_buf(&h->wait_requid_pool, wait_requid_seg);
    if(rbuf){
        memcpy(rbuf, &requidptr, sizeof(reqid13_t));
        m13_pool_unlock(&h->wait_requid_pool, wait_requid_seg);
    }
    //m13_pool_set(&h->wait_requid_pool, wait_requid_seg, &requidptr);

    th13_mutex_unlock(&h->reply_mx);

    while(h->reply_fifo.cur_requid != req_id){

        th13_cond_wait(&h->reply_cond, &h->reply_mx);

    }

    *reply = h->reply_fifo.cur;
    h->reply_fifo.cur_requid = REQID13_INVAL;

    th13_mutex_unlock(&h->reply_mx);

    return ret;
}

error13_t rr13_reg(struct rr13 *h, struct rr13_request *request, objid13_t *req_id, int8_t flags){

    struct rr13_request* req;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    e13_cleanup(&h->err);

    if(!(request->flags & RR13_REQ_COPY)) {
        req = request;
    } else {
        if(h->flags & RR13_MEMPOOL){
            req = (struct rr13_request*)m13_pool_alloc(&h->request_pool, &request->poolseg);
            if(!req){
                e13_copy(&h->err, &h->request_pool.err);
                return e13_errcode(&h->err);
            }
        } else {
            req = (struct rr13_request*)m13_malloc(sizeof(struct rr13_request));
            if(!req) return e13_ierror(&h->err, E13_NOMEM, "s", "while"
                                       "allocating memmory for request");
        }

        memcpy(req, request, sizeof(struct rr13_request));
    }

    req->next = NULL;

    th13_mutex_lock(&h->request_mx);

    if((*req_id = _rr13_get_free_reqid(h)) == RR13_REQID_INVAL){
        th13_mutex_unlock(&h->request_mx);
        return e13_ierror(&h->err, E13_NOMEM, "s","while allocating memmory / "
                                                "or ? for reqid_buf");
    }

    if(h->flags & RR13_NO_NEW_REQ){
        th13_mutex_unlock(&h->request_mx);
        return e13_ierror(&h->err, E13_READONLY, "s","No_New_Request flag set");
    }

    if(!(flags & RR13_PAUSE) && !(h->flags & RR13_PAUSE)){

        if(!h->request_fifo.first){

            h->request_fifo.first = req;
            h->request_fifo.last = req;
            h->request_fifo.n = 1UL;

        } else {

            h->request_fifo.last->next = req;
            h->request_fifo.last = req;
            h->request_fifo.n++;

        }

        th13_sem_post(&h->request_sem);

    } else {

        if(!h->wait_list.first){

            h->wait_list.first = req;
            h->wait_list.last = req;
            h->wait_list.n = 1UL;

        } else {

            h->wait_list.last->next = req;
            h->wait_list.last = req;
            h->wait_list.n++;

        }

    }

    th13_mutex_unlock(&h->request_mx);

    return E13_OK;
}

error13_t rr13_checkout(struct rr13* h, objid13_t req_id, struct rr13_reply** reply){

    struct rr13_reply* prev;
    error13_t ret;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    ret = e13_cleanup(&h->err);

    th13_mutex_lock(&h->reply_mx);

    prev = h->reply_fifo.first;

    for(*reply = h->reply_fifo.first; *reply; *reply = (*reply)->next){

        if((*reply)->reqid == req_id){

            if(*reply == h->reply_fifo.first){
                h->reply_fifo.first = (*reply)->next;
            } else if(*reply == h->reply_fifo.last){
                h->reply_fifo.last = prev;
            } else {
                prev->next = (*reply)->next;
            }

            //h->reply_fifo.new_req_id = REQID13_INVAL;

            break;
        }

        prev = *reply;

    }
    if(!(*reply)) ret = e13_ierror(&h->err, E13_NOTFOUND, "s",
                        "the reply was not found in the queue.");

    th13_mutex_unlock(&h->reply_mx);

    return ret;
}

error13_t rr13_cancel(struct rr13 *h, objid13_t req_id, struct rr13_request** req){

    struct rr13_request* prev, *request;
    error13_t ret;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    ret = e13_cleanup(&h->err);

    th13_mutex_lock(&h->request_mx);

    if(h->request_fifo.first && h->request_fifo.cur_req_id){
        ret = e13_ierror(&h->err, E13_BUSY, "s", "currently processing this!");
        goto end;
    }

    prev = h->request_fifo.first;

    for(request = h->request_fifo.first; request; request = request->next){

        if(request->reqid == req_id){

            if(request == h->request_fifo.first){
                h->request_fifo.first = request->next;
            } else if(request == h->request_fifo.last){
                h->request_fifo.last = prev;
            } else {
                prev->next = request->next;
            }

            //h->request_fifo.new_req_id = REQID13_INVAL;

            break;
        }

        prev = request;

    }

    if(!request) ret = e13_ierror(&h->err, E13_NOTFOUND, "s",
                        "interesting! the request was not found in the queue.");

end:

    th13_mutex_unlock(&h->request_mx);

    if(ret == E13_OK){
        *req = request;
    }

    return ret;

}

error13_t rr13_set_proc_flag(struct rr13* h, reqid13_t reqid, int8_t flags){

    struct rr13_request* req;
    error13_t ret;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    ret = e13_cleanup(&h->err);

    th13_mutex_lock(&h->request_mx);

    for(req = h->request_fifo.first; req; req = req->next){

        if(req->reqid == reqid){

            req->flags = flags;
            break;

        }

    }

    if(!req) ret = e13_ierror(&h->err, E13_NOTFOUND, "s", "reqid not found to"
                                                            " resume");

    th13_mutex_unlock(&h->request_mx);

    return ret;

}

error13_t rr13_resume(struct rr13 *h, reqid13_t reqid){

    struct rr13_request* req, *prev;
    error13_t ret;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    ret = e13_cleanup(&h->err);

    th13_mutex_lock(&h->request_mx);

    for(req = h->wait_list.first; req; req = req->next){

        if(req->reqid == reqid){

            if(req == h->wait_list.first){
                h->wait_list.first = h->wait_list.first->next;
            } else if(req == h->wait_list.last) {
                h->wait_list.last = prev;
            } else {
                prev->next = req->next;
            }

            if(!h->request_fifo.first){

                h->request_fifo.first = req;
                h->request_fifo.last = req;
                h->request_fifo.n = 1UL;

            } else {

                h->request_fifo.last->next = req;
                h->request_fifo.last = req;
                h->request_fifo.n++;

            }

            th13_sem_post(&h->request_sem);

            break;
        }

        prev = req;
    }

    if(!req) ret = e13_ierror(&h->err, E13_NOTFOUND, "s", "request pointer not"
                                                        " found to resume");

    th13_mutex_unlock(&h->request_mx);

    return ret;
}

error13_t rr13_pause(struct rr13 *h, reqid13_t reqid){

    struct rr13_request* req, *prev;
    error13_t ret;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    ret = e13_cleanup(&h->err);

    th13_mutex_lock(&h->request_mx);

    for(req = h->request_fifo.first; req; req = req->next){

        if(req->reqid == reqid){

            if(req == h->request_fifo.first){
                h->request_fifo.first = h->request_fifo.first->next;
            } else if(req == h->request_fifo.last) {
                h->request_fifo.last = prev;
            } else {
                prev->next = req->next;
            }

            if(!h->wait_list.first){

                h->wait_list.first = req;
                h->wait_list.last = req;
                h->wait_list.n = 1UL;

            } else {

                h->wait_list.last->next = req;
                h->wait_list.last = req;
                h->wait_list.n++;

            }

            break;
        }

        prev = req;
    }

    th13_mutex_unlock(&h->request_mx);

    return ret;
}

error13_t rr13_alloc_reply(struct rr13* h, struct rr13_reply** reply){

    msegid13_t seg;
//    error13_t ret;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    e13_cleanup(&h->err);

    if(h->flags & RR13_MEMPOOL){
        *reply = (struct rr13_reply*)m13_pool_alloc(&h->reply_pool, &seg);
        if(*reply){
            (*reply)->poolseg = seg;
        } else {
            e13_copy(&h->err, &h->reply_pool.err);
            return e13_errcode(&h->err);
        }
    } else {
        *reply = (struct rr13_reply*)malloc(sizeof(struct rr13_reply));
        if(!(*reply)) return e13_ierror(&h->err, E13_NOMEM, "s", "error"
                                        "allocating memmory for reply");
    }

    return E13_OK;

}

error13_t rr13_free_reply(struct rr13 *h, struct rr13_reply *reply){

//    error13_t ret;

    if(!_rr13_is_init(h)){
        return e13_error(E13_MISUSE);
    }

    e13_cleanup(&h->err);

    if(h->flags & RR13_MEMPOOL){
        m13_pool_free(&h->reply_pool, reply->poolseg);
    } else {
        free(reply);
    }

    return E13_OK;
}
