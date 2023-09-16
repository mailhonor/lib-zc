/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#include "zc.h"
#include "mime.h"

static inline __attribute__((always_inline)) char *ignore_chs(char *p, int plen, const char *chs_raw, int len, int flag)
{
    char *chs = (char *)(void *)chs_raw;
    int i = 0, j;
    if (plen < 1) {
        return (NULL);
    }
    if (flag < 0) {
        p += plen - 1;
    }
    for (i = 0; i < plen; i++) {
        for (j = 0; j < len; j++) {
            if (*(p) == chs[j])
                break;
        }
        if (j == len) {
            return (p);
        }
        p += flag;
    }
    return (NULL);
}

static inline __attribute__((always_inline)) char *find_delim(char *p, int plen, const char *chs_raw, int len)
{
    char *chs = (char *)(void *)chs_raw;
    int i, j;
    for (i = 0; i < plen; i++) {
        for (j = 0; j < len; j++) {
            if (p[i] == chs[j]) {
                return (p + i);
            }
        }
    }
    return (NULL);
}

/* ################################################################## */
static int find_next_kv(char *buf, int len, char **key, int *key_len, char **value, int *value_len, char **nbuf, int *nlen)
{
    char *p = buf, *p1, *p2, *pe, *pn = 0;
    int find;

    p = ignore_chs(p, len, ";\t \r\n", 4, 1);
    if (!p) {
        return -1;
    }
    *nbuf = 0;
    *key = p;

    pe = (char *)memchr(p, '=', len - (p - buf));
    if (!pe) {
        return -1;
    }
    *pe = 0;

    p1 = find_delim(p, pe - p, "\t \r\n", 4);
    if (p1) {
        *key_len = strlen(p);
    } else {
        *key_len = pe -p;
    }

    p = pe + 1;
    p = ignore_chs(p, len - (p - buf), "\t \r\n", 4, 1);
    if (!p) {
        return -1;
    }
    find = 0;
    if (*p == '"') {
        p++;
        find = 1;
    }
    *value = p;
    if (find) {
        p2 = find_delim(p, len - (p - buf), "\"\r\n", 3);
    } else {
        p2 = find_delim(p, len - (p - buf), "\t ;\r\n", 5);
    }
    if (p2) {
        *value_len = p2 - p;
        pn = p2 + 1;
    } else {
#if 0
        *value_len = strlen(p);
#endif
        *value_len = len - (p - buf);
    }

    if (pn) {
        *nbuf = pn;
        *nlen = len - (pn - buf);
    }

    return 0;
}

static int find_value(char *buf, int len, char **value, int *value_len, char **nbuf, int *nlen)
{
    char *p = buf, *p1;

    p = ignore_chs(p, len, "\t \"", 3, 1);
    if (!p) {
        return -1;
    }
    *nbuf = 0;
    *value = p;
    p1 = find_delim(p, len - (p - buf), ";\t \"\r\n", 6);
    if (p1) {
        *value_len = p1 - p;
        *nbuf = p1 + 1;
        *nlen = len - (p1 + 1 - buf);
    } else {
        *value_len = len - (p - buf);
    }

    return 0;
}

void zmime_header_line_get_params(const char *in_line, int in_len, zbuf_t *val, zdict_t *params)
{
    char *value, *nbuf;
    int value_len, nlen;
    
    zbuf_reset(val);
    if (find_value((char *)(void *)in_line, in_len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        zbuf_memcpy(val, value, value_len);
    }

    if (nbuf == 0) {
        return;
    }

    char *start, *key;
    int start_len, key_len;
    zbuf_t *kbuf = zbuf_create(64);

    start = nbuf;
    start_len = nlen;

    while (1) {
        if (start_len < 2) {
            break;
        }
        if (find_next_kv(start, start_len, &key, &key_len, &value, &value_len, &nbuf, &nlen) ) {
            break;
        }
        if (key_len < 1) {
            continue;
        }
        zbuf_memcpy(kbuf, key, key_len);
        zdict_update_string(params, zbuf_data(kbuf), value, value_len);

        if (nbuf == 0) {
            break;
        }
        start = nbuf;
        start_len = nlen;
    }

    zbuf_free(kbuf);
}

/* */
void zmime_header_line_decode_content_type_inner(zmail_t *parser, const char *data, int len, char **_value, char **boundary, int *boundary_len, char **charset, char **name)
{
    char *value, *nbuf;
    int value_len, nlen;

    if (find_value((char *)data, (int)len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        *_value = zmpool_memdupnull(parser->mpool, value, value_len);
    }

    if (nbuf == 0) {
        return;
    }

    char *start, *key;
    int start_len, key_len;

    start = nbuf;
    start_len = nlen;

    while (1) {
        if (start_len < 2) {
            break;
        }
        if (find_next_kv(start, start_len, &key, &key_len, &value, &value_len, &nbuf, &nlen) ) {
            break;
        }
        if (key_len==8 && ZSTR_N_CASE_EQ(key, "boundary", key_len)) {
            *boundary = zmpool_memdupnull(parser->mpool, value, value_len);
            *boundary_len = value_len;
        } else if (key_len == 7 && ZSTR_N_CASE_EQ(key, "charset", key_len)) {
            *charset = zmpool_memdupnull(parser->mpool, value, value_len);
        } else if (key_len == 4 && ZSTR_N_CASE_EQ(key, "name", key_len)) {
            *name = zmpool_memdupnull(parser->mpool, value, value_len);
        }
        if (nbuf == 0) {
            break;
        }
        start = nbuf;
        start_len = nlen;
    }
}

/* inner use */
void zmime_header_line_decode_content_disposition_inner(zmail_t *parser, const char *data, int len, char **_value, char **filename, char **filename_2231, int *filename_2231_with_charset_flag)
{
    char *value, *nbuf;
    int value_len, nlen;

    if (find_value((char *)(void *)data, len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        *_value = zmpool_memdupnull(parser->mpool, value, value_len);
    }

    if (nbuf == 0) {
        return;
    }

    zbuf_t *filename2231_tmpbf = 0;
    char *start, *key;
    int start_len, key_len;
    int flag_2231 = 0;
    int charset_2231 = 0;

    start = nbuf;
    start_len = nlen;

    while (1) {
        if (start_len < 2) {
            break;
        }
        if (find_next_kv(start, start_len, &key, &key_len, &value, &value_len, &nbuf, &nlen) ) {
            break;
        }
        if (key_len== 8 && ZSTR_N_CASE_EQ(key, "filename", key_len)) {
            *filename = zmpool_memdupnull(parser->mpool, value, value_len);
        } else if ((key_len > 8) && (ZSTR_N_CASE_EQ(key, "filename*", 9))) {
            if (filename2231_tmpbf == 0) {
                filename2231_tmpbf = zmail_zbuf_cache_require(parser, 1024);
            }
            zbuf_memcat(filename2231_tmpbf, value, value_len);
            if (!flag_2231)
            {
                flag_2231 = 1;
                if (key_len == 9) {
                    int count = 0;
                    for (int i = 0; i < value_len; i++) {
                        if (((unsigned char *)value)[i] == '\'') {
                            count++;
                        }
                    }
                    if (count > 1) {
                        charset_2231 = 1;
                    } else {
                        charset_2231 = 0;
                    }
                } else if (key_len == 10) {
                    charset_2231 = 0;
                } else if (key_len == 11) {
                    charset_2231 = 1;
                }
            }
        }

        if (nbuf == 0) {
            break;
        }
        start = nbuf;
        start_len = nlen;
    }
    *filename_2231_with_charset_flag = charset_2231;
    if (filename2231_tmpbf) {
        *filename_2231 = zmpool_memdupnull(parser->mpool, zbuf_data(filename2231_tmpbf), zbuf_len(filename2231_tmpbf));
        zmail_zbuf_cache_release(parser, filename2231_tmpbf);
    }
}

/* inner use */
void zmime_header_line_decode_content_transfer_encoding_inner(zmail_t *parser, const char *data, int len, char **_value)
{
    char *value, *nbuf;
    int value_len, nlen;

    if (find_value((char *)data, (int)len, &value, &value_len, &nbuf, &nlen)) {
        return;
    }
    if (value_len) {
        *_value = zmpool_memdupnull(parser->mpool, value, value_len);
    }
}
