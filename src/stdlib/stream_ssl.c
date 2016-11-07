/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-19
 * ================================
 */

#include "libzc.h"

static int ___ssl_read(zstream_t * fp, void *buf, int len, int timeout)
{
    zssl_t *ssl;

    ssl = (zssl_t *) (fp->io_ctx);

    return zssl_read(ssl, buf, len, timeout);
}

static int ___ssl_write(zstream_t * fp, void *buf, int len, int timeout)
{
    zssl_t *ssl;

    ssl = (zssl_t *) (fp->io_ctx);

    return zssl_write(ssl, buf, len, timeout);
}

zstream_t *zfopen_SSL(zssl_t * ssl)
{
    zstream_t *fp;

    fp = zstream_create(0);
    zfset_ioctx(fp, ssl, ___ssl_read, ___ssl_write);

    return fp;
}

zssl_t *zfclose_SSL(zstream_t * fp)
{
    return (zssl_t *) zstream_free(fp);
}
