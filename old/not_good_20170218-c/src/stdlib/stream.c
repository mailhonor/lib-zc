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

    zstream_flush(fp);
    r = fp->io_ctx;
    zfree(fp);

    return r;
}

void zstream_set_ioctx(zstream_t * fp, void *io_ctx, zstream_read_t read_fn, zstream_write_t write_fn)
{
    fp->io_ctx = io_ctx;
    fp->read_fn = read_fn;
    fp->write_fn = write_fn;
}

void zstream_set_timeout(zstream_t * fp, int timeout)
{
    fp->timeout = ztimeout_set(timeout);
}

/* ################################################################## */
/* read */

static int zstream_read_raw_data(zstream_t * fp)
{
    int ret;
    long time_left;

    if (fp->read_buf_p2 > fp->read_buf_p1) {
        return fp->read_buf_p2 - fp->read_buf_p1;
    }

    ZSTREAM_FLUSH(fp);

    if (ZSTREAM_ERROR(fp)) {
        return -1;
    }
    if (ZSTREAM_EOF(fp)) {
        return 0;
    }

    time_left = ztimeout_left(fp->timeout);
    if (time_left < 1) {
        fp->error = 1;
        return -1;
    }
    fp->read_buf_p1 = fp->read_buf_p2 = 0;
    ret = fp->read_fn(fp, fp->read_buf, ZSTREAM_RBUF_SIZE, time_left);
    if (ret == 0) {
        fp->eof = 1;
        return 0;
    }
    if (ret < 0) {
        fp->error = 1;
        return -1;
    }
    fp->read_buf_p2 = ret;

    return ret;
}

int zstream_getchar(zstream_t * fp)
{
    if (fp->read_buf_p2 <= fp->read_buf_p1) {
        if (zstream_read_raw_data(fp) < 0) {
            return -1;
        }
    }
    if (fp->read_buf_p2 <= fp->read_buf_p1) {
        return -1;
    }

    return ZSTREAM_GETCHAR(fp);
}

int zstream_read(zstream_t * fp, void *buf, int len)
{
    char *p, *p2;
    int i, ret, rlen;

    if (fp->read_buf_p1 >= fp->read_buf_p2) {
        ret = zstream_read_raw_data(fp);
        if (ret < 0) {
            return -1;
        } else if (ret == 0) {
            return 0;
        }
    }

    rlen = fp->read_buf_p2 - fp->read_buf_p1;
    if (len > rlen) {
        len = rlen;
    }

    p = (char *)buf;
    p2 = fp->read_buf + fp->read_buf_p1;
    i = 0;
    while (i++ < len) {
        *p++ = *p2++;
    }
    fp->read_buf_p1 += len;

    return len;
}

int zstream_read_n(zstream_t * fp, void *buf, int len)
{
    char *p;
    int i, ch;

    p = (char *)buf;
    for (i = 0; i < len; i++) {
        ch = ZSTREAM_GETCHAR(fp);
        if (ch < 0) {
            return -1;
        }
        *p++ = ch;
    }

    return len;
}

int zstream_read_delimiter(zstream_t * fp, void *buf, int len, int delimiter)
{
    char *p;
    int i, ch;

    i = -1;
    p = (char *)buf;
    for (i = 0; i < len; i++) {
        ch = ZSTREAM_GETCHAR(fp);
        if (ch < 0) {
            return -1;
        }
        *p++ = ch;
        if (ch == (int)delimiter) {
            return (i + 1);
        }
    }

    return len;
}

int zstream_gets_n(zstream_t * fp, zbuf_t * bf, int len)
{
    int i, ch;

    for (i = 0; i < len; i++) {
        ch = ZSTREAM_GETCHAR(fp);
        if (ch < 0) {
            return -1;
        }
        ZBUF_PUT(bf, ch);
    }
    ZBUF_TERMINATE(bf);

    return len;
}

int zstream_gets_delimiter(zstream_t * fp, zbuf_t * bf, int delimiter)
{
    int count, ch;

    count = 0;
    while (1) {
        ch = ZSTREAM_GETCHAR(fp);
        if (ch < 0) {
            return -1;
        }
        ZBUF_PUT(bf, ch);
        count++;
        if (ch == (int)delimiter) {
            return count;
        }
    }
    ZBUF_TERMINATE(bf);

    return -1;
}

/* ################################################################## */
/* write */

int zstream_flush(zstream_t * fp)
{
    int ret;
    long time_left;
    char *data;
    int data_len;
    int wlen;

    if (fp->write_buf_len < 1) {
        return 0;
    }

    data = fp->write_buf;
    data_len = fp->write_buf_len;
    wlen = 0;
    while (wlen < data_len) {
        time_left = ztimeout_left(fp->timeout);
        if (time_left < 1) {
            fp->error = 1;
            return -1;
        }
        ret = fp->write_fn(fp, data + wlen, data_len - wlen, time_left);
        if (ret < 1) {
            fp->error = 1;
            return -1;
        }
        wlen += ret;
    }
    fp->write_buf_len = 0;

    return data_len;
}

void zstream_putchar(zstream_t * fp, int ch)
{
    ZSTREAM_FLUSH(fp);
    ZSTREAM_PUTCHAR(fp, ch);
}

int zstream_write_n(zstream_t * fp, void *buf, int len)
{
    int ch;
    char *p;
    int wlen;

    wlen = len;
    p = (char *)buf;
    while ((wlen--) > 0) {
        ch = *p;
        ZSTREAM_PUTCHAR(fp, ch);
        if (ZSTREAM_EXCEPTION(fp)) {
            return -1;
        }
        p++;
    }

    return (len);
}

int zstream_puts(zstream_t * fp, const char *s)
{
    int i, ch;

    i = 0;
    while ((ch = (*s++))) {
        ZSTREAM_PUTCHAR(fp, ch);
        if (ZSTREAM_EXCEPTION(fp)) {
            return -1;
        }
        i++;
    }

    return i;
}

int zstream_printf_1024(zstream_t * fp, const char *format, ...)
{
    char buf[1024+1];
    va_list ap;
    int len;

    va_start(ap, format);
    len = zvsnprintf(buf, 1024, format, ap);
    va_end(ap);
    zstream_write_n(fp, buf, len);

    return len;
}

/* ################################################################## */
/* io/fd */

static int ___io_read(zstream_t * fp, void *buf, int len, int timeout)
{
    ztype_convert_t ct;

    ct.ptr_void = fp->io_ctx;

    return ztimed_read(ct.i_int, buf, len, timeout);
}

static int ___io_write(zstream_t * fp, void *buf, int len, int timeout)
{
    ztype_convert_t ct;

    ct.ptr_void = fp->io_ctx;

    return ztimed_write(ct.i_int, buf, len, timeout);
}

zstream_t *zstream_open_FD(int fd)
{
    zstream_t *fp;
    ztype_convert_t ct;

    ct.i_int = fd;

    fp = zstream_create(0);
    zstream_set_ioctx(fp, ct.ptr_void, ___io_read, ___io_write);

    return fp;
}

int zstream_close_FD(zstream_t * fp)
{
    ztype_convert_t ct;

    ct.ptr_void = zstream_free(fp);

    return ct.i_int;
}
