/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-19
 * ================================
 */

#include "zc.h"

static int ___ssl_read(zstream_t * fp, void *buf, int len, long timeout)
{
    return zopenssl_read((SSL *)(fp->io_ctx), buf, len, timeout);
}

static int ___ssl_write(zstream_t * fp, const void *buf, int len, long timeout)
{
    return zopenssl_write((SSL *)(fp->io_ctx), buf, len, timeout);
}

zstream_t *zstream_open_SSL(SSL * ssl)
{
    zstream_t *fp;

    fp = zstream_create();
    zstream_set_ioctx(fp, ssl, ___ssl_read, ___ssl_write);

    return fp;
}

SSL *zstream_close_SSL(zstream_t * fp)
{
    return (SSL *) zstream_free(fp);
}

