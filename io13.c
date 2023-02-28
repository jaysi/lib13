/*
 * $Id: hard.c, hard* functions, goin' over EINTR
 */

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/time.h>
#ifdef __WIN32
#include <io.h>
#endif
#include <fcntl.h>

#include "include/io13i.h"
#include "include/path13.h"
#include "include/debug13.h"
#include "include/mem13.h"
#include "include/str13.h"
#include "include/pack13.h"

#ifdef _WIN32
#define pipe(handles) _pipe(handles, 0, 0)
#endif

#define _deb1 _DebugMsg

#define _io13_is_init(h)        ((h)->magic==MAGIC13_IO13?1:0)
#define _io13_zero_iodone(h)    MACRO( (h)->wrdone = 0UL;(h)->rddone = 0UL; )
#define _io13_zero_lastiodone(h)    MACRO( (h)->lastwr = 0UL;(h)->lastrd = 0UL; )
#ifdef _WIN32
#define _io13_convert_offlags(h)    MACRO( (h)->sys_offlags = (h)->offlags;\
                                    (h)->sys_offlags |= O_BINARY; )
#else
#define _io13_convert_offlags(h) MACRO( (h)->sys_offlags = (h)->offlags; )
#endif

error13_t io13_open_file(struct io13* h, char* path){
    int creatperm = S_IREAD|S_IWRITE;
    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);
    h->path = NULL;
    h->fd = open(path, h->offlags, creatperm);
    if(h->fd == -1) return e13_error(E13_SYSE);
    h->path = (char*)m13_malloc(s13_strlen(path, MAXPATHNAME)+1);
    if(!h->path){
        close(h->fd);
        return e13_error(E13_NOMEM);
    }

    s13_strcpy(h->path, path, s13_strlen(path, MAXPATHNAME)+1);

    h->mode = IO13_MODE_FILE;
    return E13_OK;
}

error13_t io13_open_pipe(struct io13* rd, struct io13* wr){
    int fd[2];
    if(!_io13_is_init(rd) || !_io13_is_init(wr)) return e13_error(E13_MISUSE);
    rd->path = NULL;
    wr->path = NULL;

    if(pipe(fd) == -1){
        return e13_error(E13_SYSE);
    }

    rd->fd = fd[0];
    wr->fd = fd[1];
    rd->mode = IO13_MODE_PIPE;
    wr->mode = IO13_MODE_PIPE;

    return E13_OK;
}

error13_t io13_truncate(char* path, io13_fileoff_t off){

#ifdef truncate
# ifdef LIB13_BIG_FILES
    return !truncate64(path, off)?E13_OK:E13_IOERR;
# else
    return !truncate(path, off)?E13_OK:E13_IOERR;
# endif
#else
# ifdef LIB13_BIG_FILES
    int fd = open(path, O_WRONLY);
    if(fd == -1 || ftruncate64(fd, off) == -1){
        close(fd);
        return E13_IOERR;
    }
    close(fd);
    return E13_OK;
# else
    int fd = open(path, O_WRONLY);
    if(fd == -1 || ftruncate(fd, off) == -1){
        close(fd);
        return E13_IOERR;
    }
    close(fd);
    return E13_OK;
# endif
#endif

}

error13_t io13_init(struct io13* h, io13_offlag_t offlags){
    if(_io13_is_init(h)) return e13_error(E13_MISUSE);
    h->fd = -1;
    h->offlags = offlags;
    _io13_convert_offlags(h);
    _io13_zero_iodone(h);
    _io13_zero_lastiodone(h);
    h->mode = IO13_MODE_NONE;
    h->magic = MAGIC13_IO13;
    return E13_OK;
}

error13_t io13_destroy(struct io13 *h){
    if(h->mode == IO13_MODE_FILE) close(h->fd);
    if(h->path) m13_free(h->path);
    h->magic = MAGIC13_INV;
    return E13_OK;
}

error13_t io13_close_file(struct io13* h){
    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);
    close(h->fd);
    m13_free(h->path);
    h->mode = IO13_MODE_NONE;
    return E13_OK;
}

error13_t io13_set_ioflags(struct io13* h, io13_ioflag_t iof, enum io13_dir dir){

    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);

    switch(dir){
        case IO13_DIR_ALL:
            h->rdflags = iof;
            h->wrflags = iof;
            break;
        case IO13_DIR_RD:
            h->rdflags = iof;
            break;
        case IO13_DIR_WR:
            h->wrflags = iof;
            break;
        default:
            return e13_error(E13_MISUSE);
    }

    return E13_OK;
}

error13_t io13_seek(struct io13 *h, io13_fileoff_t offset, io13_fileoff_base_t base){
    if(seek13(h->fd, offset, base) == ((io13_fileoff_t)-1)){
        return e13_error(E13_IOERR);
    }
    return E13_OK;
}

error13_t io13_read(struct io13* h, io13_dataptr_t buf, io13_datalen_t count, io13_ioflag_t flags)
{
    //ssize_t h->iodone = 0;

    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);

    //_io13_zero_iodone(h);

    while (h->rddone < count) {
        int i;

        i = read(h->fd, buf + h->rddone, count - h->rddone);
        h->lastrd = i;
        if (i < 0) {
            if (((errno == EAGAIN) || (errno == EINTR))){
                if(h->rdflags & IO13_IOF_HARD || flags & IO13_IOF_HARD) continue;
                else return E13_CONTINUE;
            }
            return e13_error(E13_IOERR);
        }
        if (i == 0){
            return E13_DONE;
        }

        h->rddone += i;
    }
    return E13_OK;
}

error13_t io13_read_packed(struct io13* h, io13_dataptr_t* buf, io13_datalen_t* count, io13_ioflag_t flags)
{
    //ssize_t h->iodone = 0;
    io13_dataptr_t tmp;
    io13_datalen_t tmpsize;
    io13_data_t early[IO13_PACKED_EARLY_BUFSIZE];

    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);

    //_io13_zero_iodone(h);
    //lastrd keeps total

    if(!h->rddone){
        h->lastrd = IO13_PACKED_EARLY_BUFSIZE;
        tmp = early;
        tmpsize = IO13_PACKED_EARLY_BUFSIZE;
        *count = 0UL;
    } else {
        tmp = *buf + h->rddone;
        tmpsize = h->lastrd - h->rddone;//TODO: is it true?
    }

    while (h->rddone < h->lastrd) {

        int i;

        i = read(h->fd, tmp, h->lastrd - h->rddone);
        //h->lastrd = i;
        if (i < 0) {
            if (((errno == EAGAIN) || (errno == EINTR))){
                if(h->rdflags & IO13_IOF_HARD || flags & IO13_IOF_HARD) continue;
                else return E13_CONTINUE;
            }
            return e13_error(E13_IOERR);
        }

        if (i == 0){

            if(h->rddone != h->lastrd){
                return e13_error(E13_TOOSMALL);
            }

        }

        if(!h->rddone){
            i -= sizeof(uint32_t);
            if(unpack13((io13_udataptr_t)tmp, tmpsize, "L", &h->lastrd) ==
				((size_t)-1)){
				return e13_error(E13_TOOSMALL);
            }
            if(!h->lastrd) return e13_error(E13_EMPTY);
            *buf = m13_malloc(h->lastrd);
            memcpy(*buf, tmp + sizeof(uint32_t), i);
            if(!(*buf)) return e13_error(E13_NOMEM);
        }

        h->rddone += i;
        *count = h->rddone;

        if(h->rddone == h->lastrd) return E13_DONE;

    }

    return E13_OK;
}

error13_t io13_set_offlags(struct io13* h, io13_offlag_t flags){
    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);
    h->offlags = flags;
    _io13_convert_offlags(h);
    return E13_OK;
}

error13_t io13_write(struct io13* h, io13_dataptr_t buf, io13_datalen_t count, io13_ioflag_t flags)
{
    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);

    //_io13_zero_iodone(h);

    while (h->wrdone < count) {
        int i;

        i = write(h->fd, buf + h->wrdone, count - h->wrdone);
        h->lastwr = i;
        if (i < 0) {
            if ((errno == EAGAIN) || (errno == EINTR)){
                if(h->wrflags & IO13_IOF_HARD || flags & IO13_IOF_HARD) continue;
                else return E13_CONTINUE;
            }
            return e13_error(E13_IOERR);
        }

        if (i == 0){
            return E13_DONE;
        }

        h->wrdone += i;
    }
#ifndef _WIN32
    fsync(h->fd);
#else
    _commit(h->fd);
#endif
    return E13_OK;
}

error13_t io13_write_packed(struct io13* h, io13_dataptr_t buf, io13_datalen_t count, io13_ioflag_t flags)
{
    io13_dataptr_t tmp;
    io13_datalen_t tmpsize;
    io13_data_t early[IO13_PACKED_EARLY_BUFSIZE];

    if(!_io13_is_init(h)) return e13_error(E13_MISUSE);

    //_io13_zero_iodone(h);
    //lastrd keeps total

    if(!h->wrdone){
        h->lastwr = count + sizeof(uint32_t) > IO13_PACKED_EARLY_BUFSIZE ? IO13_PACKED_EARLY_BUFSIZE : count + sizeof(uint32_t);
        tmp = early;
        tmpsize = IO13_PACKED_EARLY_BUFSIZE;
        if(pack13((io13_udataptr_t)tmp, tmpsize, "L", count) == ((size_t)-1)){
			return e13_error(E13_TOOSMALL);
        }
        memcpy(tmp + sizeof(uint32_t), buf, h->lastwr - sizeof(uint32_t));
    } else {
        h->lastwr = count;
        tmp = buf + h->wrdone;
    }

    //_io13_zero_iodone(h);

    while (h->wrdone < h->lastwr) {

        int i;

        i = write(h->fd, tmp, h->lastwr - h->wrdone);
        //h->lastwr = i;
        if (i < 0) {
            if (((errno == EAGAIN) || (errno == EINTR))){
                if(h->wrflags & IO13_IOF_HARD || flags & IO13_IOF_HARD) continue;
                else return E13_CONTINUE;
            }
            return e13_error(E13_IOERR);
        }

        if (i == 0){

            if(h->wrdone != h->lastwr){
                return e13_error(E13_TOOSMALL);
            }

        }

        if(!h->wrdone){
            i -= sizeof(uint32_t);
            memcpy(buf, tmp + sizeof(uint32_t), i);
        }

        h->wrdone += i;

        if(h->wrdone == h->lastwr){
#ifndef _WIN32
            fsync(h->fd);
#else
            _commit(h->fd);
#endif
            return E13_DONE;
        }

    }

    if(h->wrflags & IO13_IOF_HARD || flags & IO13_IOF_HARD){
#ifndef _WIN32
    fsync(h->fd);
#else
    _commit(h->fd);
#endif
    }

    return E13_OK;
}

error13_t io13_stat_file(struct io13_filestat* s, char* path){
    struct stat st;
    if(stat(path, &st) == -1){
        return e13_error(E13_SYSE);
    }
    s->size = st.st_size;
    return E13_OK;
}

error13_t io13_reset(struct io13* h){
    _io13_zero_iodone(h);
    _io13_zero_lastiodone(h);
    return E13_OK;
}

int io13_copy_file2(char* target, char* source, char* extbuf,
                    io13_datalen_t extbufsize, io13_copyflag_t copyflags,
                    void*(callback(void* callback_handle, float percent))){
    struct io13_filestat st;
    //int infd, outfd;
    struct io13 inio, outio;
    io13_dataptr_t buf;
    io13_datalen_t bufsize;
    error13_t ret;
    //void*(progress_callback(void* handle, float percent)) = callback;
    io13_offlag_t outflags;

    if((ret = io13_stat_file(&st, source)) != E13_OK) return ret;

    if((ret = io13_init(&inio, IO13_IOF_DEF)) != E13_OK) return ret;

    if((ret = io13_set_offlags(&inio, IO13_OFFLAG_RDONLY)) != E13_OK){
        io13_destroy(&inio);
        return ret;
    }

    if((ret = io13_open_file(&inio, source)) != E13_OK){
        io13_destroy(&inio);
        return ret;
    }

    if((ret = io13_init(&outio, IO13_IOF_DEF)) != E13_OK) return ret;

    outflags = IO13_OFFLAG_WRONLY|IO13_OFFLAG_CREATE;
    if(copyflags & IO13_COPY_REPLACE) outflags |= IO13_OFFLAG_TRUNC;
    else outflags |= IO13_OFFLAG_EXCL;

    if((ret = io13_set_offlags(&outio, IO13_OFFLAG_WRONLY|IO13_OFFLAG_CREATE))
        != E13_OK){
        io13_destroy(&inio);
        io13_destroy(&outio);
        return ret;
    }

    if((ret = io13_open_file(&outio, target)) != E13_OK){
        io13_destroy(&inio);
        io13_destroy(&outio);
        return ret;
    }

    if((ret = io13_set_ioflags(&inio, IO13_IOF_HARD, IO13_DIR_RD)) != E13_OK){
        io13_destroy(&inio);
        io13_destroy(&outio);
        return ret;
    }

    if((ret = io13_set_ioflags(&outio, IO13_IOF_HARD, IO13_DIR_WR)) != E13_OK){
        io13_destroy(&inio);
        io13_destroy(&outio);
        return ret;
    }

    if(!extbufsize){

        if(st.size < MAXCOPYFILEBUFSIZE){
            bufsize = st.size;
        } else {
            bufsize = MAXCOPYFILEBUFSIZE;
        }

        buf = (char*)m13_malloc(bufsize);
        if(!buf){
            io13_destroy(&inio);
            io13_destroy(&outio);
            _deb1("failed here");
            return e13_error(E13_NOMEM);
        }

    } else {
        buf = extbuf;
        bufsize = extbufsize;
    }

    while( io13_get_wrsize(&outio) < st.size ){

        ret = io13_read(&inio, buf, bufsize, 0);

        switch(ret){
            case E13_OK:
                break;
            case E13_DONE:
                break;
            default:
                io13_destroy(&inio);
                io13_destroy(&outio);
                return ret;
        }

        ret = io13_write(&outio, buf, inio.lastrd, 0);

        switch(ret){
            case E13_OK:
                break;
            case E13_DONE:
                break;
            default:
                io13_destroy(&inio);
                io13_destroy(&outio);
                return ret;
        }

        if(callback) callback(NULL, io13_get_wrsize(&outio)/st.size);

    }

    if(!extbufsize){
        m13_free(buf);
    }

    io13_destroy(&inio);
    io13_destroy(&outio);

    if(copyflags & IO13_COPY_MOVE){
        io13_remove_file(source);
    }

    return 0;

}

int io13_copy_file(char* target, char* source, io13_copyflag_t copyflags){

    char buf[COPYFILEBUFSIZE];

    return io13_copy_file2( target, source, buf,
                            COPYFILEBUFSIZE, copyflags, NULL);

}
