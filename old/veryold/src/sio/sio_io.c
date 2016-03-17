#include "zc.h"

static ssize_t ___io_read(ZSIO * fp, void *buf, size_t len, int timeout)
{
	int fd;

	fd = ZVOID_PTR_TO_INT((fp->io_ctx));

	return zio_timed_read(fd, buf, len, timeout);
}

static ssize_t ___io_write(ZSIO * fp, void *buf, size_t len, int timeout)
{
	int fd;

	fd = ZVOID_PTR_TO_INT((fp->io_ctx));

	return zio_timed_write(fd, buf, len, timeout);
}

int zsio_set_FD(ZSIO * fp, int fd)
{
	return zsio_set_ioctx(fp, ZINT_TO_VOID_PTR(fd), ___io_read, ___io_write);
}

int zsio_get_FD(ZSIO * fp)
{
	return (ZVOID_PTR_TO_INT((fp->io_ctx)));
}
