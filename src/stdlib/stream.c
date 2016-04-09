/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-19
 * ================================
 */

#include "libzc.h"

zstream_t *zstream_create(int unused)
{
    zstream_t *fp;

    fp = (zstream_t *) zcalloc(1, sizeof(zstream_t));
    fp->io_ctx = 0;
    fp->timeout = ztimeout_set(1000L * 3600 * 24 * 100);

    return fp;
}

void *zstream_free(zstream_t * fp)
{
    void *r;

    zfflush(fp);
    r = fp->io_ctx;
    zfree(fp);

    return r;
}

void zfset_ioctx(zstream_t * fp, void *io_ctx, zstream_read_t read_fn, zstream_write_t write_fn)
{
    fp->io_ctx = io_ctx;
    fp->read_fn = read_fn;
    fp->write_fn = write_fn;
}

void zfset_timeout(zstream_t * fp, int timeout)
{
    fp->timeout = ztimeout_set(timeout);
}

/* ################################################################## */
/* read */

static int zfread_raw_data(zstream_t * fp)
{
    int ret;
    long time_left;

    if (fp->read_buf_p2 > fp->read_buf_p1)
    {
        return fp->read_buf_p2 - fp->read_buf_p1;
    }

    ZFFLUSH(fp);

    if (ZFERROR(fp))
    {
        return -1;
    }
    if (ZFEOF(fp))
    {
        return 0;
    }

    time_left = ztimeout_left(fp->timeout);
    if (time_left < 1)
    {
        fp->error = 1;
        return -1;
    }
    fp->read_buf_p1 = fp->read_buf_p2 = 0;
    ret = fp->read_fn(fp, fp->read_buf, ZFILE_RBUF_SIZE, time_left);
    if (ret == 0)
    {
        fp->eof = 1;
        return 0;
    }
    if (ret < 0)
    {
        fp->error = 1;
        return -1;
    }
    fp->read_buf_p2 = ret;

    return ret;
}

int zfgetchar(zstream_t * fp)
{
    if (fp->read_buf_p2 <= fp->read_buf_p1)
    {
        if (zfread_raw_data(fp) < 0)
        {
            return -1;
        }
    }
    if (fp->read_buf_p2 <= fp->read_buf_p1)
    {
        return -1;
    }

    return ZFGETCHAR(fp);
}

int zfread(zstream_t * fp, void *buf, int len)
{
    char *p, *p2;
    int i, ret, rlen;

    if (fp->read_buf_p1 >= fp->read_buf_p2)
    {
        ret = zfread_raw_data(fp);
        if (ret < 0)
        {
            return -1;
        }
        else if (ret == 0)
        {
            return 0;
        }
    }

    rlen = fp->read_buf_p2 - fp->read_buf_p1;
    if (len > rlen)
    {
        len = rlen;
    }

    p = (char *)buf;
    p2 = fp->read_buf + fp->read_buf_p1;
    i = 0;
    while (i++ < len)
    {
        *p++ = *p2++;
    }
    fp->read_buf_p1 += len;

    return len;
}

int zfread_n(zstream_t * fp, void *buf, int len)
{
    char *p;
    int i, ch;

    p = (char *)buf;
    for (i = 0; i < len; i++)
    {
        ch = ZFGETCHAR(fp);
        if (ch < 0)
        {
            return -1;
        }
        *p++ = ch;
    }

    return len;
}

int zfread_delimiter(zstream_t * fp, void *buf, int len, char delimiter)
{
    char *p;
    int i, ch;

    i = -1;
    p = (char *)buf;
    for (i = 0; i < len; i++)
    {
        ch = ZFGETCHAR(fp);
        if (ch < 0)
        {
            return -1;
        }
        *p++ = ch;
        if (ch == (int)delimiter)
        {
            return (i + 1);
        }
    }

    return len;
}

int zfgets_n(zstream_t * fp, zbuf_t * bf, int len)
{
    int i, ch;

    for (i = 0; i < len; i++)
    {
        ch = ZFGETCHAR(fp);
        if (ch < 0)
        {
            return -1;
        }
        ZBUF_PUT(bf, ch);
    }

    return len;
}

int zfgets_delimiter(zstream_t * fp, zbuf_t * bf, char delimiter)
{
    int count, ch;

    count = 0;
    while (1)
    {
        ch = ZFGETCHAR(fp);
        if (ch < 0)
        {
            return -1;
        }
        ZBUF_PUT(bf, ch);
        count++;
        if (ch == (int)delimiter)
        {
            return count;
        }
    }

    return -1;
}

/* ################################################################## */
/* write */

int zfflush(zstream_t * fp)
{
    int ret;
    long time_left;
    char *data;
    int data_len;
    int wlen;

    if (fp->write_buf_len < 1)
    {
        return 0;
    }

    data = fp->write_buf;
    data_len = fp->write_buf_len;
    wlen = 0;
    while (wlen < data_len)
    {
        time_left = ztimeout_left(fp->timeout);
        if (time_left < 1)
        {
            fp->error = 1;
            return -1;
        }
        ret = fp->write_fn(fp, data + wlen, data_len - wlen, time_left);
        if (ret < 1)
        {
            fp->error = 1;
            return -1;
        }
        wlen += ret;
    }
    fp->write_buf_len = 0;

    return data_len;
}

void zfputchar(zstream_t * fp, int ch)
{
    ZFPUTCHAR(fp, ch);
}

int zfwrite_n(zstream_t * fp, void *buf, int len)
{
    int ch;
    char *p;

    p = (char *)buf;
    while ((len--) > 0)
    {
        ch = *p;
        ZFPUTCHAR(fp, ch);
        if (ZFEXCEPTION(fp))
        {
            return -1;
        }
        p++;
    }

    return (len);
}

int zfputs(zstream_t * fp, char *s)
{
    int i, ch;

    i = 0;
    while ((ch = (*s)))
    {
        ZFPUTCHAR(fp, ch);
        if (ZFEXCEPTION(fp))
        {
            return -1;
        }
        i++;
    }

    return i;
}

int zfprintf(zstream_t * fp, char *format, ...)
{
    char buf[1024000 + 16];
    va_list ap;
    int i;

    va_start(ap, format);
    i = zvsnprintf(buf, 1024000, format, ap);
    va_end(ap);
    zfwrite_n(fp, buf, i);

    return i;
}

/* ################################################################## */
/* io/fd */

static int ___io_read(zstream_t * fp, void *buf, int len, int timeout)
{
    int fd;

    fd = ZVOID_PTR_TO_INT((fp->io_ctx));

    return ztimed_read(fd, buf, len, timeout);
}

static int ___io_write(zstream_t * fp, void *buf, int len, int timeout)
{
    int fd;

    fd = ZVOID_PTR_TO_INT((fp->io_ctx));

    return ztimed_write(fd, buf, len, timeout);
}

zstream_t *zfopen_FD(int fd)
{
    zstream_t *fp;

    fp = zstream_create(0);
    zfset_ioctx(fp, ZINT_TO_VOID_PTR(fd), ___io_read, ___io_write);

    return fp;
}

int zfclose_FD(zstream_t * fp)
{
    void *r;

    r = zstream_free(fp);

    return ZVOID_PTR_TO_INT(r);
}

/* ################################################################## */
/* ssl */

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
