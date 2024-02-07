
#pragma once

#include <sys/uio.h>
#include <root/err.h>

/*
 * The following routines are functionally equivalent to libc read{v}/write{v}
 * except how an error condition is handled. On error:
 *    read{v}/write{v} returns -1 and sets errno
 *    read(v)/write{v} return negative OLDK error instead
 */
ssize_t readv(int fd, const struct iovec* iov, int iovcnt);
ssize_t read(int fd, void* buf, size_t count);
ssize_t writev(int fd, const struct iovec* iov, int iovcnt);
ssize_t write(int fd, const void* buf, size_t count);