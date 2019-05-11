/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2016-02-23
 * ================================
 */

#include "zc.h"

int zsize_data_unescape(const void *src_data, int src_size, void **result_data, int *result_len)
{
    int i = 0;
    unsigned char *buf = (unsigned char *)src_data;
    int ch, len = 0, shift = 0;
    while (1) {
        ch = ((i++ == src_size) ? -1 : *buf++);
        if (ch == -1) {
            return -1;
        }
        len |= ((ch & 0177) << shift);
        if (ch & 0200) {
            break;
        }
        shift += 7;
    }
    if (i + len > src_size) {
        return -1;
    }
    *result_data = buf;
    *result_len = len;
    return i + len;
}

int zsize_data_unescape_all(const void *src_data, int src_size, zsize_data_t *vec, int vec_size)
{
    int count = 0;
    zsize_data_t *sd;
    unsigned char *buf = (unsigned char *)src_data;
    while (1) {
        if (count >=  vec_size) {
            return count;
        }
        int i = 0;
        int ch, len = 0, shift = 0;
        while (1) {
            ch = ((i++ == src_size) ? -1 : *buf++);
            if (ch == -1) {
                return count;
            }
            len |= ((ch & 0177) << shift);
            if (ch & 0200) {
                break;
            }
            shift += 7;
        }
        if (i + len> src_size) {
            return count;
        }
        sd = vec + count++;
        sd->data = (char *)buf;
        sd->size = len;
        buf += len;
    }
    return count;
}

void zsize_data_escape(zbuf_t * zb, const void *data, int len)
{
	int ch, left = len;

	if (len < 0) {
        len = strlen((const char *)data);
	}
	do {
		ch = left & 0177;
		left >>= 7;
		if (!left) {
			ch |= 0200;
		}
		ZBUF_PUT(zb, ch);
	} while (left);
	if (len > 0) {
		zbuf_memcat(zb, data, len);
	}
}

void zsize_data_escape_int(zbuf_t * zb, int i)
{
	char buf[32];
	int len;
	len = sprintf(buf, "%d", i);
	zsize_data_escape(zb, buf, len);
}

void zsize_data_escape_long(zbuf_t * zb, long i)
{
	char buf[64];
	int len;
	len = sprintf(buf, "%lu", i);
	zsize_data_escape(zb, buf, len);
}

void zsize_data_escape_dict(zbuf_t * zb, zdict_t * zd)
{
	ZDICT_WALK_BEGIN(zd, k, v) {
        zsize_data_escape(zb, k, -1);
        zsize_data_escape(zb, zbuf_data(v), zbuf_len(v));
    } ZDICT_WALK_END;
}

void zsize_data_escape_pp(zbuf_t * zb, const char **pp, int size)
{
    for (int i = 0;i<size;i++) {
        zsize_data_escape(zb, pp[i], -1);
    }
}

int zsize_data_put_size(int size, char *buf)
{
    int ch, left = size, len = 0;
	do {
		ch = left & 0177;
		left >>= 7;
		if (!left) {
			ch |= 0200;
		}
        ((unsigned char *)buf)[len++] = ch;
	} while (left);
    return len;
}

int zsize_data_get_size_from_zstream(zstream_t *fp)
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
