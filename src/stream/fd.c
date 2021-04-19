/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-19
 * ================================
 */

#include "zc.h"

typedef struct ssl_ioctx_t ssl_ioctx_t;
struct ssl_ioctx_t {
    SSL *ssl;
    int fd;
};

static const char *fp_get_type()
{
    return "_Z_FD";
}

static int fp_close(zstream_t *fp, int release_ioctx)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    int ret = 0;
    if (release_ioctx) {
        ret = zclose(ioctx->fd);
    }
    zfree(ioctx);

    return ret;
}

static int fp_read(zstream_t *fp, void *buf, int len)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ztimed_read(ioctx->fd, buf, len, fp->read_wait_timeout);
}

static int fp_write(zstream_t *fp, const void *buf, int len)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ztimed_write(ioctx->fd, buf, len, fp->write_wait_timeout);
}

static int fp_timed_read_wait(zstream_t *fp, int read_wait_timeout)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ztimed_read_wait(ioctx->fd, read_wait_timeout);
}

static int fp_timed_write_wait(zstream_t *fp, int write_wait_timeout)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ztimed_write_wait(ioctx->fd, write_wait_timeout);
}

static int fp_get_fd(zstream_t *fp)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ioctx->fd;
}

static zstream_engine_t fp_engine = {
    fp_get_type,
    fp_close,
    fp_read,
    fp_write,
    fp_timed_read_wait,
    fp_timed_write_wait,
    fp_get_fd
};

zstream_t *zstream_open_fd_engine(zstream_t *fp, int fd)
{
    memset(fp, 0, sizeof(zstream_t));
    fp->read_buf_p1 = 0;
    fp->read_buf_p2 = 0;
    fp->write_buf_len = 0;
    fp->read_wait_timeout = -1;
    fp->write_wait_timeout = -1;
    fp->error = 0;
    fp->eof = 0;
    fp->engine = &fp_engine;
    fp->ioctx = (ssl_ioctx_t *)zcalloc(1, sizeof(ssl_ioctx_t));
    ((ssl_ioctx_t *)(fp->ioctx))->fd = fd;
    return fp;
}

zstream_t *zstream_open_fd(int fd)
{
    zstream_t *fp = (zstream_t *)zmalloc(sizeof(zstream_t));
    return zstream_open_fd_engine(fp, fd);
}

zstream_t *zstream_open_destination(const char *destination, int timeout)
{
    int fd = zconnect(destination, timeout);
    if (fd < 0) {
        return 0;
    }
    znonblocking(fd, 1);
    return zstream_open_fd(fd);
}

