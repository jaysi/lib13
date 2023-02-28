#include "include/io13i.h"

error13_t _io13_seek_write(struct io13 *h, io13_dataptr_t buf, io13_datalen_t len, io13_fileoff_t offset)
{
    error13_t ret;
    if ((ret=io13_seek(h, offset, IO13_FILEOFF_BASE_START)) != E13_OK)
        return ret;
    io13_set_ioflags(h, IO13_IOF_HARD, IO13_DIR_WR);
    if ((ret = io13_write(h, buf, len, 0)) != E13_OK)
        return ret;
    return E13_OK;
}

error13_t _io13_seek_read(struct io13 *h, io13_dataptr_t buf, io13_datalen_t len, io13_fileoff_t offset)
{
    error13_t ret;
    if ((ret=io13_seek(h, offset, IO13_FILEOFF_BASE_START)) != E13_OK)
        return ret;
    io13_set_ioflags(h, IO13_IOF_HARD, IO13_DIR_RD);
    if ((ret = io13_read(h, buf, len, 0)) != E13_OK)
        return ret;
    return E13_OK;
}

