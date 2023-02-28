#ifndef IO13I_H
#define IO13I_H

#include "io13.h"

#define COPYFILEBUFSIZE (1024*1024) //default copy buffer, 1mb

error13_t _io13_seek_write(struct io13* h, char *buf, io13_datalen_t len, io13_fileoff_t offset);
error13_t _io13_seek_read(struct io13* h, char *buf, io13_datalen_t len, io13_fileoff_t offset);

#endif // IO13I_H
