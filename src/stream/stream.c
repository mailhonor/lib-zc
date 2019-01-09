/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-19
 * ================================
 */

#include "zc.h"
#include <fcntl.h>
#include <errno.h>

#define _ZSTREAM_INIT(fp)  \
    fp->read_buf_p1 = 0; \
    fp->read_buf_p2 = 0; \
    fp->write_buf_len = 0; \
    fp->error = 0; \
    fp->eof = 0; \
    fp->ssl_mode = 0; \
    fp->file_mode = 0;

zstream_t *zstream_open_fd(int fd)
{
    zstream_t *fp = (zstream_t *)zmalloc(sizeof(zstream_t));
    _ZSTREAM_INIT(fp);
    fp->cutoff_time = 999999999999L;
    fp->ioctx.fd = fd;
    return fp;
}

zstream_t *zstream_open_ssl(SSL *ssl)
{
    zstream_t *fp = (zstream_t *)zmalloc(sizeof(zstream_t));
    _ZSTREAM_INIT(fp);
    fp->cutoff_time = 999999999999L;
    fp->ioctx.ssl = ssl;
    fp->ssl_mode = 1;
    return fp;
}

zstream_t *zstream_open_destination(const char *destination, int timeout)
{
    int fd = zconnect(destination, 1, timeout);
    if (fd == -1) {
        return 0;
    }
    return zstream_open_fd(fd);
}

zstream_t *zstream_open_file(const char *pathname, const char *mode)
{
    int flags = 0;
    if (*mode == 'r') {
        flags = O_RDONLY;
        if (mode[1] == '+') {
            flags = O_RDWR;
        }
    } else  if (*mode == 'w') {
        flags = O_WRONLY|O_TRUNC|O_CREAT;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT;
        }
    } else  if (*mode == 'a') {
        flags = O_WRONLY|O_TRUNC|O_CREAT|O_APPEND;
        if (mode[1] == '+') {
            flags = O_RDWR|O_TRUNC|O_CREAT|O_APPEND;
        }
    } else {
        flags = O_RDONLY;
    }
    int fd = open(pathname, flags, 0666);
    if (fd == -1) {
        return 0;
    }
    zstream_t *fp = (zstream_t *)zmalloc(sizeof(zstream_t));
    _ZSTREAM_INIT(fp);
    fp->cutoff_time = 999999999999L;
    fp->ioctx.fd = fd;
    fp->file_mode = 1;
    return fp;

}

int zstream_close(zstream_t *fp, int close_fd_and_release_ssl)
{
    int ret=zstream_flush(fp);
    if (close_fd_and_release_ssl) {
        if (fp->ssl_mode) {
            SSL *ssl = fp->ioctx.ssl;
            int fd = zopenssl_SSL_get_fd(ssl);
            zopenssl_SSL_free(ssl);
            zclose(fd);
        } else {
            zclose(fp->ioctx.fd);
        }
    }
    zfree(fp);
    return ret;
}

void zstream_set_timeout(zstream_t *fp, int timeout)
{
    fp->cutoff_time = ztimeout_set(timeout);
}

int zstream_get_fd(zstream_t *fp)
{
    if (fp->ssl_mode) {
        return zopenssl_SSL_get_fd(fp->ioctx.ssl);
    } else {
        return fp->ioctx.fd;
    }
}

SSL *zstream_get_ssl(zstream_t *fp)
{
    if (fp->ssl_mode) {
        return fp->ioctx.ssl;
    } else {
        return 0;
    }
}

int zstream_timed_read_wait(zstream_t *fp, int timeout)
{
    if (fp->read_buf_p1 < fp->read_buf_p2) {
        return 1;
    }
    return ztimed_read_wait(fp->ssl_mode?zopenssl_SSL_get_fd(fp->ioctx.ssl):fp->ioctx.fd, timeout);
}

int zstream_timed_write_wait(zstream_t *fp, int timeout)
{
    return ztimed_write_wait(fp->ssl_mode?zopenssl_SSL_get_fd(fp->ioctx.ssl):fp->ioctx.fd, timeout);
}

int zstream_tls_connect(zstream_t *fp, SSL_CTX *ctx)
{
    if (fp->ssl_mode) {
        return -1;
    }

    SSL *_ssl = zopenssl_SSL_create(ctx, fp->ioctx.fd);
    if (!_ssl) {
        return -1;
    }
    int timeout = ztimeout_left(fp->cutoff_time);
    if (timeout < 1) {
        return -1;
    }
    if (zopenssl_timed_connect(_ssl, timeout) < 0) {
        zopenssl_SSL_free(_ssl);
        fp->error = 1;
        return -1;
    }
    fp->ssl_mode = 1;
    fp->ioctx.ssl = _ssl;
    return 0;
}

int zstream_tls_accept(zstream_t *fp, SSL_CTX *ctx)
{
    if (fp->ssl_mode) {
        return -1;
    }

    SSL *_ssl = zopenssl_SSL_create(ctx, fp->ioctx.fd);
    if (!_ssl) {
        return -1;
    }
    int timeout = ztimeout_left(fp->cutoff_time);
    if (timeout < 1) {
        return -1;
    }
    if (zopenssl_timed_accept(_ssl, timeout) < 0) {
        zopenssl_SSL_free(_ssl);
        fp->error = 1;
        return -1;
    }
    fp->ssl_mode = 1;
    fp->ioctx.ssl = _ssl;
    return 0;
}

/* read */
int zstream_getc_do(zstream_t *fp)
{
    if (fp->read_buf_p1 < fp->read_buf_p2) {
        return fp->read_buf[fp->read_buf_p1++];
    }

    if (fp->error || fp->eof) {
        return -1;
    }

    if (fp->write_buf_len > 0) {
        if (zstream_flush(fp) == -1) {
            return -1;
        }
    }

    fp->read_buf_p1 = fp->read_buf_p2 = 0;

    int ret = -1, ec;
    if (fp->file_mode) {
        for (;;) {
            ret = read(fp->ioctx.fd, fp->read_buf, zvar_stream_rbuf_size);
            if (ret < 0) {
                ec = errno;
                if (ec == EINTR) {
                    if (zvar_proc_stop) {
                        break;
                    }
                    continue;
                }
                if (ec == EAGAIN) {
                    zfatal("zfstream_t: can not use nonblocking fd");
                    ret = -1;
                    break;
                }
                break;
            } else if (ret == 0) {
                break;
            } else {
                break;
            }
        }
    } else if (fp->ssl_mode) {
        int timeout = ztimeout_left(fp->cutoff_time);
        if (timeout < 1) {
            return -1;
        }
        ret = zopenssl_timed_read(fp->ioctx.ssl, fp->read_buf, zvar_stream_rbuf_size, timeout);
    } else {
        long cutoff_time = fp->cutoff_time * 1000, left_time = ztimeout_left_millisecond(cutoff_time);
        for (;left_time>0;left_time=ztimeout_left_millisecond(cutoff_time)) {
            if (ztimed_read_wait_millisecond(fp->ioctx.fd, left_time) < 1) {
                ret = -1;
                break;
            }
            if ((ret = read(fp->ioctx.fd, fp->read_buf, zvar_stream_rbuf_size)) < 0) {
                ec = errno;
                if (ec == EINTR) {
                    if (zvar_proc_stop) {
                        break;
                    }
                    continue;
                }
                if (ec == EAGAIN) {
                    continue;
                }
                ret = -1;
                break;
            }
            break;
        }
    }

    if (ret == 0) {
        fp->eof = 1;
        return -1;
    }
    if (ret < 0) {
        fp->error = 1;
        return -1;
    }
    fp->read_buf_p1 = 1;
    fp->read_buf_p2 = ret;

    return fp->read_buf[0];
}

void zstream_ungetc(zstream_t *fp)
{
    if (fp->read_buf_p1 > 0) {
        fp->read_buf_p1--;
    } else {
        zfatal("zstream_ungetc too much");
    }
}

int zstream_read(zstream_t *fp, zbuf_t *bf, int max_len)
{
    if (max_len < 1) {
        return 0;
    }
    zbuf_reset(bf);

    int left_len = max_len;
    int ch;
    int have_len = fp->read_buf_p2 - fp->read_buf_p1;
    if (have_len == 0) {
        ch = zstream_getc_do(fp);
        if (ch == -1) {
            if (fp->eof) {
                return 0;
            }
            return -1;
        }
        ZBUF_PUT(bf, ch);
        have_len = fp->read_buf_p2 - fp->read_buf_p1;
        left_len --;
    }
    zbuf_terminate(bf);
    if (left_len > have_len) {
        left_len = have_len;
    }
    zbuf_memcat(bf, fp->read_buf, left_len);
    fp->read_buf_p1 += left_len;
    return zbuf_len(bf);
}

int zstream_readn(zstream_t *fp, zbuf_t *bf, int strict_len)
{
    if (strict_len < 1) {
        return 0;
    }
    if (bf) {
        zbuf_reset(bf);
    }
    
    int left_len = strict_len;
    int ch;

    if (bf) {
        while (left_len > 0) {
            ch = ZSTREAM_GETC(fp);
            if (ch == -1) {
                break;
            }
            ZBUF_PUT(bf, ch);
            left_len--;
        }
    } else {
        while (left_len > 0) {
            ch = ZSTREAM_GETC(fp);
            if (ch == -1) {
                break;
            }
            left_len--;
        }
    }
    if (bf) {
        zbuf_terminate(bf);
    }

    if (strict_len > left_len) {
        return strict_len - left_len;
    }
    if (fp->eof) {
        return 0;
    }
    return -1;
}

int zstream_gets_delimiter(zstream_t *fp, zbuf_t *bf, int delimiter, int max_len)
{
    if (max_len < 1) {
        return 0;
    }
    if (bf) {
        zbuf_reset(bf);
    }

    int left_len = max_len;
    int ch;

    if (bf) {
        while(left_len > 0) {
            ch = ZSTREAM_GETC(fp);
            if (ch == -1) {
                break;
            }
            ZBUF_PUT(bf, ch);
            left_len --;
            if (ch == delimiter) {
                break;
            }
        }
    } else {
        while(left_len > 0) {
            ch = ZSTREAM_GETC(fp);
            if (ch == -1) {
                break;
            }
            left_len --;
            if (ch == delimiter) {
                break;
            }
        }
    }
    if (bf) {
        zbuf_terminate(bf);
    }
    if (max_len > left_len) {
        return max_len - left_len;
    }
    if (fp->eof) {
        return 0;
    }
    return -1;
}

int zstream_size_data_get_size(zstream_t *fp)
{
    int ch, size = 0, shift = 0;
    while (1) {
        ch = ZSTREAM_GETC(fp);
        if (ch == -1) {
            return -1;
        }
        size |= ((ch & 0177) << shift);
        if (ch & 0200) {
            break;
        }
        shift += 7;
    }
    return size;
}

/* write */
int zstream_flush(zstream_t *fp)
{
    if (fp->error) {
        return -1;
    }
    int ret, left = fp->write_buf_len, ec;
    char *data = (char *)(fp->write_buf);
    if (fp->file_mode) {
        while(left > 0) {
            ret = write(fp->ioctx.fd, data, left);
            if (ret < 0) {
                ec = errno;
                if (ec == EINTR) {
                    if (zvar_proc_stop) {
                        break;
                    }
                    continue;
                }
                if (ec == EPIPE) {
                    break;
                }
                break;
            } else if (ret == 0) {
                continue;
            } else {
                left -= ret;
                data += ret;
            }
        }
    } else if (fp->ssl_mode) {
        while(left > 0) {
            int timeout = ztimeout_left(fp->cutoff_time);
            if (timeout < 1) {
                break;
            }
            ret = zopenssl_timed_write(fp->ioctx.ssl, data+(fp->write_buf_len-left), left, timeout);
            if (ret < 1) {
                break;
            }
            left -= ret;
        }
    } else {
        long cutoff_time = fp->cutoff_time * 1000, left_time = left_time=ztimeout_left_millisecond(cutoff_time);
        for (;(left>0) && (left_time>0);left_time=ztimeout_left_millisecond(cutoff_time)) {
            if (ztimed_write_wait_millisecond(fp->ioctx.fd, left_time) < 1) {
                break;
            }
            ret = write(fp->ioctx.fd, data, left);
            if (ret < 0) {
                ec = errno;
                if (ec == EINTR) {
                    if (zvar_proc_stop) {
                        break;
                    }
                    continue;
                }
                if (ec == EAGAIN) {
                    continue;
                }
                if (ec == EPIPE) {
                }
                break;
            } else if (ret == 0) {
                continue;
            } else {
                left -= ret;
                data += ret;
            }
        }
    }

    fp->write_buf_len = 0;
    if (left > 0) {
        fp->error = 1;
        return -1;
    }
    return 1;
}

int zstream_putc_do(zstream_t *fp, int ch)
{
    int ret;

    if (fp->error) {
        return -1;
    }

    if (fp->write_buf_len >= zvar_stream_rbuf_size) {
        ret = zstream_flush(fp);
        if (ret == -1) {
            return -1;
        }
    }
    fp->write_buf[fp->write_buf_len++] = ch;
    return ch;
}

int zstream_puts(zstream_t *fp, const char *s)
{
    int len = 0;
    int ch;
    if (s == 0) {
        s = "";
    }
    for (;*s;s++) {
        len++;
        ch = *s;
        ZSTREAM_PUTC(fp, ch);
        if (fp->error || fp->eof) {
            return -1;
        }
    }
    return len;
}

int zstream_write(zstream_t *fp, const void *buf, int len)
{
    if (len < 1) {
        return 0;
    }
    int left_len = len;
    int ch;
    const char *str = (const char *)buf;
    if (str == 0) {
        str = "";
    }
    while (left_len--) {
        ch = *str++;
        ZSTREAM_PUTC(fp, ch);
        if (fp->error || fp->eof) {
            return -1;
        }
    }
    return len;
}

int zstream_printf_1024(zstream_t *fp, const char *format, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    if (format == 0) {
        format = "";
    }
    va_start(ap, format);
    len = vsnprintf(buf, 1024, format, ap);
    len = ((len<1024)?len:(1024-1));
    va_end(ap);

    return zstream_write(fp, buf, len);
}

int zstream_write_size_data_size(zstream_t *fp, int size)
{
    int ch, left = size, len = 0;
	do {
		ch = left & 0177;
		left >>= 7;
		if (!left) {
			ch |= 0200;
		}
        ZSTREAM_PUTC(fp, ch);
	} while (left);
    return len;
}

int zstream_write_size_data(zstream_t *fp, const void *buf, int len)
{
    if (len < 0) {
        len = strlen((const char *)buf);
    }
    zstream_write_size_data_size(fp, len);
    zstream_write(fp, buf, len);
    return len;
}

int zstream_write_size_data_int(zstream_t *fp, int i)
{
	char buf[32];
	int len;
	len = sprintf(buf, "%d", i);
    zstream_write_size_data_size(fp, len);
    zstream_write(fp, buf, len);
    return len;
}

int zstream_write_size_data_long(zstream_t *fp, long i)
{
	char buf[64];
	int len;
	len = sprintf(buf, "%lu", i);
    zstream_write_size_data_size(fp, len);
    zstream_write(fp, buf, len);
    return len;
}

int zstream_write_size_data_dict(zstream_t *fp, zdict_t * zd)
{
	ZDICT_WALK_BEGIN(zd, k, v) {
        zstream_write_size_data(fp, k, 1);
        zstream_write_size_data(fp, zbuf_data(v), zbuf_len(v));
    } ZDICT_WALK_END;
    return 1;
}

int zstream_write_size_data_pp(zstream_t *fp, const char **pp, int size)
{
    for (int i = 0;i<size;i++) {
        zstream_write_size_data(fp, pp[i], -1);
    }
    return 1;
}
