#include "zc.h"

static ssize_t ___io_read(ZSIO * fp, void *buf, size_t len, int timeout)
{
	ZSSL *ssl;

	ssl = (ZSSL *) (fp->io_ctx);

	return zssl_read(ssl, buf, len, timeout);
}

static ssize_t ___io_write(ZSIO * fp, void *buf, size_t len, int timeout)
{
	ZSSL *ssl;

	ssl = (ZSSL *) (fp->io_ctx);

	return zssl_write(ssl, buf, len, timeout);
}

int zsio_set_SSL(ZSIO * fp, ZSSL * ssl)
{
	return zsio_set_ioctx(fp, ssl, ___io_read, ___io_write);
}
