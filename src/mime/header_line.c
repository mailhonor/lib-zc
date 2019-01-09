/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "zc.h"
#include "mime.h"

void zmime_raw_header_line_unescape(const char *in_line, int in_len, zbuf_t *result)
{
    int ch;
    char *src = (char *)(void *)(in_line);
    int i;

    zbuf_reset(result);
    for (i = 0; i < in_len; i++) {
        ch = src[i];
        if (ch == '\0') {
            continue;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            i++;
            if (i == in_len) {
                break;
            }
            ch = src[i];
            if (ch == '\t') {
                continue;
            }
            if (ch == ' ') {
                continue;
            }
            ZBUF_PUT(result, '\n');
        }
        ZBUF_PUT(result, ch);
    }
    zbuf_terminate(result);
}

int zmime_raw_header_line_unescape_inner(zmail_t *parser, const char *data, size_t size, char *dest, int dest_size)
{
    int ch;
    char *src = (char *)(void *)data;
    size_t i, rlen = 0;

    if (size > dest_size){
        size = dest_size;
    }
    for (i = 0; i < size; i++) {
        ch = src[i];
        if (ch == '\0') {
            continue;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            i++;
            if (i == size) {
                break;
            }
            ch = src[i];
            if (ch == '\t') {
                continue;
            }
            if (ch == ' ') {
                continue;
            }
            dest[rlen++] = '\n';
        }
        dest[rlen++] = ch;
    }

    return rlen;
}

int zmime_header_line_get_first_token_inner(const char *line_, int in_len, char **val)
{
    int i, vlen, ch, len = in_len;
    char *line = (char *)(void *)line_, *ps, *pend = line + len;

    *val = line;
    vlen = 0;
    for (i = 0; i < len; i++) {
        ch = line[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '<')) {
            continue;
        }
        break;
    }
    if (i == len) {
        return vlen;
    }
    ps = line + i;
    len = pend - ps;
    for (i = len - 1; i >= 0; i--) {
        ch = ps[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '>')) {
            continue;
        }
        break;
    }
    if (i < 0) {
        return vlen;
    }

    *val = ps;
    vlen = i + 1;
    return vlen;
}

void zmime_header_line_get_first_token(const char *in_line, int in_len, zbuf_t *result)
{
    char *v;
    int l = zmime_header_line_get_first_token_inner(in_line, in_len, &v);
    zbuf_reset(result);
    if (l>0) {
        zbuf_memcat(result, v, l);
    }
}

const zvector_t *zmime_header_line_get_element_vector(const char *in_line, int in_len)
{
    if (in_len == -1) {
        in_len = strlen(in_line);
    }
    if (in_len < 1) {
        return 0;
    }
    char *ps = (char *)(void *)in_line, *in_end = ps + in_len;
    char *p1, *p2, *p3, *p, *pf, *pf_e, *pch, *pch_e, *pen, *pdata, *pdata_e;
    zmime_header_line_element_t *mt;
    int tmp_len;

    zvector_t *element_vector = zvector_create(10);

#define element_vector_add_one_element(mt) { \
    if (mt==0) { \
        mt = (zmime_header_line_element_t *)zmalloc(sizeof(zmime_header_line_element_t)); \
        mt->charset = zblank_buffer; \
        zvector_add(element_vector, mt); \
    } \
}
    mt = 0;
    while (in_end > ps) {
        p = zmemstr(ps, "=?", in_end - ps);
        pf = ps;
        pf_e = p - 1;
        while (p) {
            pch = p + 2;
            p1 = zmemcasestr(pch, "?B?", in_end - pch);
            p2 = zmemcasestr(pch, "?Q?", in_end - pch);
            if (p1 && p2) {
                p3 = (p1 < p2 ? p1 : p2);
            } else if (p1 == 0) {
                p3 = p2;
            } else {
                p3 = p1;
            }
            if (!p3) {
                p = 0;
                break;
            }
            pch_e = p3 - 1;
            pen = p3 + 1;
            pdata = p3 + 3;
            p = zmemstr(pdata, "?=", in_end - pdata);
            if (!p) {
                break;
            }
            pdata_e = p - 1;
            ps = p + 2;
            element_vector_add_one_element(mt);
            mt->encode_type = 0;
            mt->dlen = pf_e - pf + 1;
            mt->data = zmemdupnull(pf, mt->dlen);
            mt = 0;
            element_vector_add_one_element(mt);
            {
                char c = ztoupper(*pen);
                if (c == 'B') {
                    mt->encode_type = 'B';
                } else if ( c == 'Q') {
                    mt->encode_type = 'Q';
               } else  {
                    mt->encode_type = 0;
                }
            }
            tmp_len = pch_e - pch + 1;
            if (tmp_len < 1) {
                mt->charset = zblank_buffer;
            } else {
                mt->charset = zmemdupnull(pch, tmp_len);
            }
            {
                /* rfc 2231 */
                char *p = strchr(mt->charset, '*');
                if (p) {
                    *p = 0;
                }
            }
            mt->dlen = pdata_e - pdata + 1;
            mt->data = zmemdupnull(pdata, mt->dlen);
            mt = 0;
            p = (char *)in_line;
            break;
        }
        if (!p) {
            element_vector_add_one_element(mt);
            mt->encode_type = 0;
            mt->dlen = strlen(ps);
            mt->data = zmemdupnull(ps, mt->dlen);
            mt = 0;
            break;
        }
    }
#undef element_vector_add_one_element

    return element_vector;
}

void zmime_header_line_element_vector_free(const zvector_t *element_vector)
{
    if (element_vector == 0) {
        return;
    }
    ZVECTOR_WALK_BEGIN(element_vector, zmime_header_line_element_t *, element) {
        zfree(element->charset);
        zfree(element->data);
        zfree(element);
    } ZVECTOR_WALK_END;
    zvector_free((zvector_t *)(void *)element_vector);
}

void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result)
{
    zbuf_reset(result);
    if (in_len == -1){
        in_len = strlen(in_line);
    }
    if (in_len < 1) {
        return;
    }
    int ret, i, plen, mt_count;
    char *in_src = (char *)(void *)in_line, *p;
    zmime_header_line_element_t *mt, *mtn;
    const zvector_t *mt_vec;

    mt_vec = zmime_header_line_get_element_vector(in_src, in_len);
    if (!mt_vec) {
        return;
    }
    mt_count = zvector_len(mt_vec);

    zbuf_t *bq_join = zbuf_create(0);
    zbuf_t *out_string = zbuf_create(0);

    for (i = 0; i < mt_count; i++) {
        mt = (zmime_header_line_element_t *)(zvector_data(mt_vec)[i]);
        if (mt->dlen == 0) {
            continue;
        }
        if ((mt->encode_type != 'B') && (mt->encode_type != 'Q')) {
            zbuf_reset(out_string);
            zmime_iconv(src_charset_def, mt->data, mt->dlen, out_string);
            zbuf_append(result, out_string);
            continue;
        }
        zbuf_memcpy(bq_join, mt->data, mt->dlen);
        mtn = (zmime_header_line_element_t *)(zvector_data(mt_vec)[i+1]);
        while (1) {
            if (i + 1 >= mt_count) {
                break;
            }
            if (mtn->encode_type == 0) {
                size_t j;
                char c;
                for (j = 0; j < mtn->dlen; j++) {
                    c = mtn->data[j];
                    if (c == ' ') {
                        continue;
                    }
                    break;
                }
                if (j == mtn->dlen) {
                    i++;
                    mtn = (zmime_header_line_element_t *)(zvector_data(mt_vec)[i+1]);
                    continue;
                }
                break;
            }
            if ((mt->encode_type == mtn->encode_type) && (*(mt->charset)) && (*(mtn->charset)) && (!strcasecmp(mt->charset, mtn->charset))) {
                zbuf_memcat(bq_join, mtn->data, mtn->dlen);
                i++;
                mtn = (zmime_header_line_element_t *)(zvector_data(mt_vec)[i+1]);
                continue;
            }
            break;
        }
        p = zbuf_data(bq_join);
        plen = zbuf_len(bq_join);
        p[plen] = 0;
        ret = 0;
        zbuf_reset(out_string);
        if (mt->encode_type == 'B') {
            ret = zbase64_decode(p, plen, out_string, 0);
        } else if (mt->encode_type == 'Q') {
            ret = zqp_decode_2047(p, plen, out_string);
        }

        if (ret < 1) {
            continue;
        }
        zbuf_memcpy(bq_join, zbuf_data(out_string), zbuf_len(out_string));
        zbuf_reset(out_string);
        zmime_iconv(mt->charset, zbuf_data(bq_join), zbuf_len(bq_join), out_string);
        zbuf_append(result, out_string);
    }
    zmime_header_line_element_vector_free(mt_vec);
    zbuf_free(bq_join);
    zbuf_free(out_string);
}

/* ###################################################### */
typedef struct zmime_header_line_element2_t zmime_header_line_element2_t;
struct zmime_header_line_element2_t {
    char *charset;
    char *data;
    int dlen;
    int clen;
    char encode_type; /* 'B':base64, 'Q':qp, 0:unknown */
};

static int zmime_header_line_get_element_vector_inner(const char *in_line, int in_len, zmime_header_line_element2_t *vec, int max_count)
{
    if (in_len == -1) {
        in_len = strlen(in_line);
    }
    if (in_len < 1) {
        return 0;
    }
    char *ps = (char *)(void *)in_line, *in_end = ps + in_len;
    char *p1, *p2, *p3, *p, *pf, *pf_e, *pch, *pch_e, *pen, *pdata, *pdata_e;
    zmime_header_line_element2_t *mt;
    int count=0, tmp_len;

    mt = 0;
    while (in_end > ps) {
        p = zmemstr(ps, "=?", in_end - ps);
        pf = ps;
        pf_e = p - 1;
        while (p) {
            pch = p + 2;
            p1 = zmemcasestr(pch, "?B?", in_end - pch);
            p2 = zmemcasestr(pch, "?Q?", in_end - pch);
            if (p1 && p2) {
                p3 = (p1 < p2 ? p1 : p2);
            } else if (p1 == 0) {
                p3 = p2;
            } else {
                p3 = p1;
            }
            if (!p3) {
                p = 0;
                break;
            }
            pch_e = p3 - 1;
            pen = p3 + 1;
            pdata = p3 + 3;
            p = zmemstr(pdata, "?=", in_end - pdata);
            if (!p) {
                break;
            }
            pdata_e = p - 1;
            ps = p + 2;
            mt = vec + count++;
            if (count > max_count) {
                return count - 1;
            }
            mt->charset = zblank_buffer;
            mt->clen = 0;
            mt->encode_type = 0;
            mt->dlen = pf_e - pf + 1;
            mt->data = pf;
            mt = vec + count++;
            if (count > max_count) {
                return count - 1;
            }
            {
                char c = ztoupper(*pen);
                if (c == 'B') {
                    mt->encode_type = 'B';
                } else if ( c == 'Q') {
                    mt->encode_type = 'Q';
               } else  {
                    mt->encode_type = 0;
                }
            }
            tmp_len = pch_e - pch + 1;
            if (tmp_len < 1) {
                mt->charset = zblank_buffer;
                mt->clen = 0;
            } else {
                mt->charset = pch;
                mt->clen = tmp_len;
            }
            if (mt->clen) {
                /* rfc 2231 */
                char *p = memchr(mt->charset, '*', mt->clen);
                if (p) {
                    *p = 0;
                    mt->clen = p - mt->charset;
                }
            }
            mt->dlen = pdata_e - pdata + 1;
            mt->data = pdata;
            mt = 0;
            p = (char *)in_line;
            break;
        }
        if (!p) {
            mt = vec + count++;
            if (count > max_count) {
                return count - 1;
            }
            mt->charset = zblank_buffer;
            mt->clen = 0;
            mt->encode_type = 0;
            mt->dlen = strlen(ps);
            mt->data = ps;
            mt = 0;
            break;
        }
    }

    return count;
}

void zmime_header_line_get_utf8_inner(zmail_t *parser, const char *in_line, int in_len, zbuf_t *result)
{
    zbuf_reset(result);
    if (in_len == -1){
        in_len = strlen(in_line);
    }
    if (in_len < 1) {
        return;
    }
    int ret, i, plen, mt_count;
    char *in_src = (char *)(void *)in_line, *p;
    zmime_header_line_element2_t *mt_vec, *mt, *mtn;

    plen = in_len -1;
    p = in_src;
    mt_count = 0;
    for (i=0;i<plen;i++, p++){
        if ((p[0] != '=') || (p[1] != '?')) {
            continue;
        }
        mt_count++;
        i++;
        p++;
    }
    mt_count = mt_count*2 + 10;
    mt_vec = (zmime_header_line_element2_t *)zmalloc(mt_count * sizeof(zmime_header_line_element2_t));
    mt_count = zmime_header_line_get_element_vector_inner(in_src, in_len, mt_vec, mt_count);
    if (!mt_count) {
        return;
    }

    zbuf_t *bq_join = zmail_zbuf_cache_require(parser, -1);
    zbuf_t *out_string = zmail_zbuf_cache_require(parser, -1);

    for (i = 0; i < mt_count; i++) {
        mt = mt_vec + i;
        if (mt->dlen == 0) {
            continue;
        }
        if ((mt->encode_type != 'B') && (mt->encode_type != 'Q')) {
            zbuf_reset(out_string);
            zmime_iconv(parser->src_charset_def, mt->data, mt->dlen, out_string);
            zbuf_append(result, out_string);
            continue;
        }
        zbuf_memcpy(bq_join, mt->data, mt->dlen);
        mtn = mt_vec + i + 1;
        while (1) {
            if (i + 1 >= mt_count) {
                break;
            }
            if (mtn->encode_type == 0) {
                size_t j;
                char c;
                for (j = 0; j < mtn->dlen; j++) {
                    c = mtn->data[j];
                    if (c == ' ') {
                        continue;
                    }
                    break;
                }
                if (j == mtn->dlen) {
                    i++;
                    mtn = mt_vec + i + 1;
                    continue;
                }
                break;
            }
            if ((mt->clen > 0) && (mt->encode_type == mtn->encode_type) && (*(mt->charset)) && (*(mtn->charset)) && (mt->clen==mtn->clen) && (!strncasecmp(mt->charset, mtn->charset, mt->clen))) {
                zbuf_memcat(bq_join, mtn->data, mtn->dlen);
                i++;
                mtn = mt_vec + i + 1;
                continue;
            }
            break;
        }
        p = zbuf_data(bq_join);
        plen = zbuf_len(bq_join);
        p[plen] = 0;
        ret = 0;
        zbuf_reset(out_string);
        if (mt->encode_type == 'B') {
            ret = zbase64_decode(p, plen, out_string, 0);
        } else if (mt->encode_type == 'Q') {
            ret = zqp_decode_2047(p, plen, out_string);
        }

        if (ret < 1) {
            continue;
        }
        zbuf_memcpy(bq_join, zbuf_data(out_string), zbuf_len(out_string));
        zbuf_reset(out_string);
        char charset[zvar_charset_name_max_size + 1];
        if (mt->clen > zvar_charset_name_max_size) {
            mt->clen = zvar_charset_name_max_size;
        }
        charset[0] = 0;
        memcpy(charset, mt->charset, mt->clen);
        charset[mt->clen] = 0;
        zmime_iconv(charset, zbuf_data(bq_join), zbuf_len(bq_join), out_string);
        zbuf_append(result, out_string);
    }
    zfree(mt_vec);
    zmail_zbuf_cache_release(parser, bq_join);
    zmail_zbuf_cache_release(parser, out_string);
}
