
#ifndef MARCEL_IO_EST_DEF
#define MARCEL_IO_EST_DEF

#include <unistd.h>
#include <sys/time.h>
#include <sys/uio.h>

void marcel_io_init(void);

int marcel_read(int fildes, void *buf, size_t nbytes);

int marcel_write(int fildes, void *buf, size_t nbytes);

int marcel_select(int nfds, fd_set *rfds, fd_set *wfds);

int marcel_readv(int fildes, const struct iovec *iov, int iovcnt);

int marcel_writev(int fildes, const struct iovec *iov, int iovcnt);

/* To force reading/writing an exact number of bytes */

int marcel_read_exactly(int fildes, void *buf, size_t nbytes);

int marcel_write_exactly(int fildes, void *buf, size_t nbytes);

int marcel_readv_exactly(int fildes, const struct iovec *iov, int iovcnt);

int marcel_writev_exactly(int fildes, const struct iovec *iov, int iovcnt);

#endif
