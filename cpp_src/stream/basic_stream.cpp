/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-15
 * ================================
 */

#include "zc.h"

namespace zcc
{

basic_stream::basic_stream()
{
    basic_worker_ = (basic_stream_worker *)zcalloc(1, sizeof(basic_stream_worker));
}

basic_stream::~basic_stream()
{
    zfree(basic_worker_);
    basic_worker_ = 0;
}

basic_stream &basic_stream::reset()
{
    memset(basic_worker_, 0, sizeof(basic_stream_worker));
    return *this;
}

int basic_stream::getc_do()
{
    if (basic_worker_->read_buf_p1 < basic_worker_->read_buf_p2) {
        return basic_worker_->read_buf[basic_worker_->read_buf_p1++];
    }

    if (basic_worker_->error || basic_worker_->eof) {
        return -1;
    }

    if (basic_worker_->write_buf_len > 0) {
        if (flush() == -1) {
            return -1;
        }
    }

    basic_worker_->read_buf_p1 = basic_worker_->read_buf_p2 = 0;

    int ret = engine_read(basic_worker_->read_buf, zvar_stream_rbuf_size);

    if (ret == 0) {
        basic_worker_->eof = 1;
        return -1;
    }
    if (ret < 0) {
        basic_worker_->error = 1;
        return -1;
    }
    basic_worker_->read_buf_p1 = 1;
    basic_worker_->read_buf_p2 = ret;

    return basic_worker_->read_buf[0];
}

basic_stream &basic_stream::ungetc()
{
    if (basic_worker_->read_buf_p1 > 0) {
        basic_worker_->read_buf_p1--;
    } else {
        zfatal("zstream_ungetc too much");
    }
    return *this;
}

int basic_stream::read(zbuf_t *bf, int max_len)
{
    if (max_len < 1) {
        return 0;
    }
    zbuf_reset(bf);

    int left_len = max_len;
    int ch;
    int have_len = basic_worker_->read_buf_p2 - basic_worker_->read_buf_p1;
    if (have_len == 0) {
        ch = getc_do();
        if (ch == -1) {
            if (basic_worker_->eof) {
                return 0;
            }
            return -1;
        }
        ZBUF_PUT(bf, ch);
        have_len = basic_worker_->read_buf_p2 - basic_worker_->read_buf_p1;
        left_len --;
    }
    zbuf_terminate(bf);
    if (left_len > have_len) {
        left_len = have_len;
    }
    zbuf_memcat(bf, basic_worker_->read_buf, left_len);
    basic_worker_->read_buf_p1 += left_len;
    return zbuf_len(bf);
}

int basic_stream::read(void *mem, int max_len)
{
    if (max_len < 1) {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = max_len, ret_len = 0;
    int ch;
    int have_len = basic_worker_->read_buf_p2 - basic_worker_->read_buf_p1;
    if (have_len == 0) {
        ch = getc_do();
        if (ch == -1) {
            if (basic_worker_->eof) {
                return 0;
            }
            return -1;
        }
        *ps++ = ch;
        have_len = basic_worker_->read_buf_p2 - basic_worker_->read_buf_p1;
        left_len --;
        ret_len ++;
    }

    if (left_len > have_len) {
        left_len = have_len;
    }
    ret_len += left_len;
    memcpy(ps, basic_worker_->read_buf, left_len);
    basic_worker_->read_buf_p1 += left_len;
    return ret_len;
}

int basic_stream::read(std::string &str, int max_len)
{
    if (max_len < 1) {
        return 0;
    }

    str.clear();
    int left_len = max_len, ret_len = 0;
    int ch;
    int have_len = basic_worker_->read_buf_p2 - basic_worker_->read_buf_p1;
    if (have_len == 0) {
        ch = getc_do();
        if (ch == -1) {
            if (basic_worker_->eof) {
                return 0;
            }
            return -1;
        }
        str.push_back(ch);
        have_len = basic_worker_->read_buf_p2 - basic_worker_->read_buf_p1;
        left_len --;
        ret_len ++;
    }

    if (left_len > have_len) {
        left_len = have_len;
    }
    ret_len += left_len;
    str.append((char *)(basic_worker_->read_buf), left_len);
    basic_worker_->read_buf_p1 += left_len;
    return ret_len;
}

int basic_stream::readn(zbuf_t *bf, int strict_len)
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
            ch = getc();
            if (ch == -1) {
                break;
            }
            ZBUF_PUT(bf, ch);
            left_len--;
        }
    } else {
        while (left_len > 0) {
            ch = getc();
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
    if (basic_worker_->eof) {
        return 0;
    }
    return -1;
}

int basic_stream::readn(void *mem, int strict_len)
{
    if (strict_len < 1) {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = strict_len;
    int ch;

    if (mem) {
        while (left_len > 0) {
            ch = getc();
            if (ch == -1) {
                break;
            }
            *ps ++ = ch;
            left_len--;
        }
    } else {
        while (left_len > 0) {
            ch = getc();
            if (ch == -1) {
                break;
            }
            left_len--;
        }
    }

    if (strict_len > left_len) {
        return strict_len - left_len;
    }
    if (basic_worker_->eof) {
        return 0;
    }
    return -1;
}

int basic_stream::readn(std::string &str, int strict_len)
{
    if (strict_len < 1) {
        return 0;
    }

    str.clear();
    int left_len = strict_len;
    int ch;

    while (left_len > 0) {
        ch = getc();
        if (ch == -1) {
            break;
        }
        str.push_back(ch);
        left_len--;
    }

    if (strict_len > left_len) {
        return strict_len - left_len;
    }
    if (basic_worker_->eof) {
        return 0;
    }
    return -1;
}

int basic_stream::read_delimiter(zbuf_t *bf, int delimiter, int max_len)
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
            ch = getc();
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
            ch = getc();
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
    if (basic_worker_->eof) {
        return 0;
    }
    return -1;
}

int basic_stream::read_delimiter(void *mem, int delimiter, int max_len)
{
    if (max_len < 1) {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = max_len;
    int ch;

    if (ps) {
        while(left_len > 0) {
            ch = getc();
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
            ch = getc();
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
    if (basic_worker_->eof) {
        return 0;
    }
    return -1;
}

int basic_stream::read_delimiter(std::string &str, int delimiter, int max_len)
{
    if (max_len < 1) {
        return 0;
    }

    str.clear();
    int left_len = max_len;
    int ch;

    while(left_len > 0) {
        ch = getc();
        if (ch == -1) {
            break;
        }
        str.push_back(ch);
        left_len --;
        if (ch == delimiter) {
            break;
        }
    }

    if (max_len > left_len) {
        return max_len - left_len;
    }
    if (basic_worker_->eof) {
        return 0;
    }
    return -1;
}

int basic_stream::write(const void *buf, int len)
{
    if (len < 0) {
        len = strlen((const char *)buf);
    }
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
        putc(ch);
        if (basic_worker_->error || basic_worker_->eof) {
            return -1;
        }
    }
    return len;
}

int basic_stream::printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    if (format == 0) {
        format = "";
    }
    va_start(ap, format);
    len = zvsnprintf(buf, 1024, format, ap);
    len = ((len<1024)?len:(1024-1));
    va_end(ap);

    return write(buf, len);
}

int basic_stream::flush()
{
    if (basic_worker_->error) {
        return -1;
    }
    int ret, left = basic_worker_->write_buf_len;
    char *data = (char *)(basic_worker_->write_buf);

    while(left > 0) {
        if ((ret = engine_write(data, left)) < 0) {
            break;
        }
        left -= ret;
        data += ret;
    }

    basic_worker_->write_buf_len = 0;
    if (left > 0) {
        basic_worker_->error = 1;
        return -1;
    }
    return 1;
}

int basic_stream::putc_do(int ch)
{
    int ret;

    if (basic_worker_->error) {
        return -1;
    }

    if (basic_worker_->write_buf_len >= zvar_stream_rbuf_size) {
        ret = flush();
        if (ret == -1) {
            return -1;
        }
    }
    basic_worker_->write_buf[basic_worker_->write_buf_len++] = ch;
    return ch;
}

int basic_stream::get_cint()
{
    int ch, size = 0, shift = 0;
    while (1) {
        ch = getc();
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
int basic_stream::write_cint(int size)
{
    int ch, left = size, len = 0;
	do {
		ch = left & 0177;
		left >>= 7;
		if (!left) {
			ch |= 0200;
		}
        putc(ch);
	} while (left);
    return len;
}

int basic_stream::write_cint_and_data(const void *buf, int len)
{
    if (len < 0) {
        len = strlen((char *)(void *)buf);
    }
    write_cint(len);
    write(buf, len);
    return len;
}

int basic_stream::write_cint_and_int(int i)
{
	char buf[32];
	int len;
	len = zsprintf(buf, "%d", i);
    write_cint(len);
    write(buf, len);
    return len;
}

int basic_stream::write_cint_and_long(ssize_t i)
{
	char buf[64];
	int len;
	len = zsprintf(buf, "%zd", i);
    write_cint_and_data(buf, len);
    return len;
}

int basic_stream::write_cint_and_dict(zdict_t * zd)
{
	ZDICT_WALK_BEGIN(zd, k, v) {
        write_cint_and_data(k, -1);
        write_cint_and_data(zbuf_data(v), zbuf_len(v));
    } ZDICT_WALK_END;
    return 1;
}

int basic_stream::write_cint_and_pp(const char **pp, int size)
{
    for (int i = 0;i<size;i++) {
        write_cint_and_data(pp[i], -1);
    }
    return 1;
}

} /* namespace zcc */

