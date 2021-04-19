/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-19
 * ================================
 */

#include "zc.h"

int zstream_close_engine(zstream_t *fp, int release_ioctx)
{
    int ret=zstream_flush(fp);
    fp->engine->close_fn(fp, release_ioctx);
    return ret;
}

int zstream_close(zstream_t *fp, int release_ioctx)
{
    int ret=zstream_flush(fp);
    fp->engine->close_fn(fp, release_ioctx);
    zfree(fp);
    return ret;
}

void zstream_set_read_wait_timeout(zstream_t *fp, int read_wait_timeout)
{
    fp->read_wait_timeout = read_wait_timeout;
}

void zstream_set_write_wait_timeout(zstream_t *fp, int write_wait_timeout)
{
    fp->write_wait_timeout = write_wait_timeout;
}

int zstream_get_fd(zstream_t *fp)
{
    return fp->engine->get_fd_fn(fp);
}

int zstream_timed_read_wait(zstream_t *fp, int timeout)
{
    if (fp->read_buf_p1 < fp->read_buf_p2) {
        return 1;
    }
    return fp->engine->timed_read_wait_fn(fp, timeout);
}

int zstream_timed_write_wait(zstream_t *fp, int timeout)
{
    return fp->engine->timed_write_wait_fn(fp, timeout);
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

    int ret = fp->engine->read_fn(fp, fp->read_buf, zvar_stream_rbuf_size);

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
        zfatal("FATAL zstream_ungetc too much");
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

int zstream_read_to_mem(zstream_t *fp, void *mem, int max_len)
{
    if (max_len < 1) {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = max_len, ret_len = 0;
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
        *ps++ = ch;
        have_len = fp->read_buf_p2 - fp->read_buf_p1;
        left_len --;
        ret_len ++;
    }

    if (left_len > have_len) {
        left_len = have_len;
    }
    ret_len += left_len;
    memcpy(ps, fp->read_buf, left_len);
    fp->read_buf_p1 += left_len;
    return ret_len;
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

int zstream_readn_to_mem(zstream_t *fp, void *mem, int strict_len)
{
    if (strict_len < 1) {
        return 0;
    }

    char *ps = (void *)mem;
    int left_len = strict_len;
    int ch;

    if (mem) {
        while (left_len > 0) {
            ch = ZSTREAM_GETC(fp);
            if (ch == -1) {
                break;
            }
            *ps ++ = ch;
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

    if (strict_len > left_len) {
        return strict_len - left_len;
    }
    if (fp->eof) {
        return 0;
    }
    return -1;
}

int zstream_read_delimiter(zstream_t *fp, zbuf_t *bf, int delimiter, int max_len)
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

int zstream_read_delimiter_to_mem(zstream_t *fp, void *mem, int delimiter, int max_len)
{
    if (max_len < 1) {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = max_len;
    int ch;

    if (ps) {
        while(left_len > 0) {
            ch = ZSTREAM_GETC(fp);
            if (ch == -1) {
                break;
            }
            *ps++ = ch;
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

    *ps = 0;
    if (max_len > left_len) {
        return max_len - left_len;
    }
    if (fp->eof) {
        return 0;
    }
    return -1;
}

int zstream_get_cint(zstream_t *fp)
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
    int ret, left = fp->write_buf_len;
    char *data = (char *)(fp->write_buf);

    while(left > 0) {
        if ((ret = fp->engine->write_fn(fp, data, left)) < 0) {
            break;
        }
        left -= ret;
        data += ret;
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

int zstream_write_cint(zstream_t *fp, int size)
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

int zstream_write_cint_and_data(zstream_t *fp, const void *buf, int len)
{
    if (len < 0) {
        len = strlen((char *)(void *)buf);
    }
    zstream_write_cint(fp, len);
    zstream_write(fp, buf, len);
    return len;
}

int zstream_write_cint_and_int(zstream_t *fp, int i)
{
	char buf[32];
	int len;
	len = sprintf(buf, "%d", i);
    zstream_write_cint(fp, len);
    zstream_write(fp, buf, len);
    return len;
}

int zstream_write_cint_and_long(zstream_t *fp, long i)
{
	char buf[64];
	int len;
	len = sprintf(buf, "%lu", i);
    zstream_write_cint_and_data(fp, buf, len);
    return len;
}

int zstream_write_cint_and_dict(zstream_t *fp, zdict_t * zd)
{
	ZDICT_WALK_BEGIN(zd, k, v) {
        zstream_write_cint_and_data(fp, k, -1);
        zstream_write_cint_and_data(fp, zbuf_data(v), zbuf_len(v));
    } ZDICT_WALK_END;
    return 1;
}

int zstream_write_cint_and_pp(zstream_t *fp, const char **pp, int size)
{
    for (int i = 0;i<size;i++) {
        zstream_write_cint_and_data(fp, pp[i], -1);
    }
    return 1;
}
