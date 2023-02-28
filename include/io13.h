#ifndef IO13_H
#define IO13_H

#include "type13.h"
#include "error13.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <stdlib.h>
#include <stdio.h>

#ifdef LIB13_BIG_FILES
typedef off64_t io13_fileoff_t;//file offset
#define seek13 lseek64
#else
typedef off_t io13_fileoff_t;//file offset
#define seek13 lseek
#endif
typedef int io13_fileoff_base_t;//whence
typedef size_t io13_datalen_t;
typedef int io13_offlag_t;//open file flags
typedef int _io13_sys_offlag_t;//open file flags
typedef uint16_t io13_ioflag_t;
typedef char* io13_dataptr_t;
typedef uchar* io13_udataptr_t;
typedef char io13_data_t;
typedef uchar io13_udata_t;
typedef uint8_t io13_copyflag_t;

#define MAXCOPYFILEBUFSIZE (10*1024*1024) //max 10mb
#define IO13_PACKED_EARLY_BUFSIZE   1024

struct io13_filestat{
    io13_datalen_t size;
};

#define IO13_FILEOFF_BASE_START SEEK_SET
#define IO13_FILEOFF_BASE_CUR   SEEK_CUR
#define IO13_FILEOFF_BASE_END   SEEK_END

#define IO13_OFFLAG_CREATE  O_CREAT
#define IO13_OFFLAG_RDONLY  O_RDONLY
#define IO13_OFFLAG_WRONLY  O_WRONLY
#define IO13_OFFLAG_RDWR    O_RDWR
#define IO13_OFFLAG_EXCL    O_EXCL
#define IO13_OFFLAG_APPEND  O_APPEND
#define IO13_OFFLAG_TRUNC   O_TRUNC

#define IO13_OFFLAG_DEF     (IO13_OFFLAG_CREATE|IO13_OFFLAG_RDWR|IO13_OFFLAG_APPEND)

enum io13_mode{
    IO13_MODE_NONE,
    IO13_MODE_FILE,
    IO13_MODE_SOCKET,
    IO13_MODE_PIPE
};

//#define IO13_MODE_NONE      0x00
//#define IO13_MODE_FILE      0x01
//#define IO13_MODE_SOCKET    0x02

enum io13_dir{
    IO13_DIR_NONE,
    IO13_DIR_WR,
    IO13_DIR_RD,
    IO13_DIR_ALL
};

#define IO13_IOF_HARD       (0x0001<<0) //read/write hard or fail
#define IO13_IOF_ASYNC      (0x0001<<1)
#define IO13_IOF_CALLBACK   (0x0001<<2)
//#define IO13_IOF_PACKED     (0x0001<<3)//packed mode, use _packed calls for compatibility issues

#define IO13_IOF_DEF    IO13_IOF_HARD

#define IO13_COPY_REPLACE   (0x01<<0)
#define IO13_COPY_MOVE      (0x01<<1)

struct io13{

    magic13_t magic;

    enum io13_mode mode;

    io13_ioflag_t rdflags;
    io13_ioflag_t wrflags;
    io13_offlag_t offlags;//open-file flags
    _io13_sys_offlag_t sys_offlags;//system open-flags
    char* path;
    int fd;

    size_t wrdone; //total size of i/o done till now
    size_t rddone; //total size of i/o done till now
    size_t lastrd;//last chunk of data read or total on packed mode
    size_t lastwr;//last chunk of data written or total on packed mode

    //this will be called upon completion if set.
    error13_t (*callback)(struct io13*, io13_dataptr_t buf, io13_datalen_t size);

    struct e13 err;

};

#ifdef __cplusplus
    extern "C" {
#endif

error13_t io13_init(struct io13* h, io13_offlag_t offlags);
error13_t io13_seek(struct io13 *h, io13_fileoff_t offset, io13_fileoff_base_t base);
error13_t io13_read(struct io13* h, io13_dataptr_t buf, io13_datalen_t count, io13_ioflag_t flags);
error13_t io13_write(struct io13* h, io13_dataptr_t buf, io13_datalen_t count, io13_ioflag_t flags);
error13_t io13_read_packed(struct io13* h, io13_dataptr_t *buf, io13_datalen_t* count, io13_ioflag_t flags);
error13_t io13_write_packed(struct io13* h, io13_dataptr_t buf, io13_datalen_t count, io13_ioflag_t flags);
error13_t io13_destroy(struct io13* h);
//error13_t io13_close(struct io13* h);
//error13_t io13_open(struct io13* h, char* path, enum io13_mode mode);
error13_t io13_open_file(struct io13* h, char* path);
error13_t io13_open_pipe(struct io13* rd, struct io13* wr);
error13_t io13_close_file(struct io13* h);
error13_t io13_reset(struct io13* h);
error13_t io13_stat_file(struct io13_filestat* s, char* path);
error13_t io13_set_ioflags(struct io13* h, io13_ioflag_t iof, enum io13_dir dir);
error13_t io13_set_offlags(struct io13* h, io13_offlag_t flags);
error13_t io13_copy_file(   char* target, char* source, io13_copyflag_t copyflags);
error13_t io13_copy_file2(  char* target, char* source, char* extbuf,
                            size_t extbufsize, io13_copyflag_t copyflags,
                            void*(callback)(void* callback_handle, float percent));
error13_t io13_truncate(char* path, io13_fileoff_t off);
#define io13_get_wrsize(h) ((h)->wrdone)
#define io13_remove_file(path) remove(path)

#ifdef __cplusplus
    }
#endif

#endif
