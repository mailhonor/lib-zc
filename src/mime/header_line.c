/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#include "zc.h"
#include "mime.h"

/* {{{ zmime_raw_header_line_unescape */
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

int zmime_raw_header_line_unescape_inner(zmail_t *parser, const char *data, int size, char *dest, int dest_size)
{
    int ch;
    char *src = (char *)(void *)data;
    int i, rlen = 0;

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
/* }}} */

/* {{{ zmime_header_line_get_first_token */
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
/* }}} */

/* {{{ zmime_header_line_get_element_vector */
typedef struct zmime_header_line_element2_t zmime_header_line_element2_t;
struct zmime_header_line_element2_t {
    const char *charset;
    const char *data;
    int dlen;
    int clen;
    char encode_type; /* 'B':base64, 'Q':qp, 0:unknown */
};

static int zmime_header_line_get_element_vector_inner(const char *in_line, int in_len, zmime_header_line_element2_t *mt_vec, int max_count)
{
    if (in_len < 0) {
        in_len = strlen(in_line);
    }
    if (in_len < 1) {
        return 0;
    }
    int count = 0, clen, dlen, found, encode;
    zmime_header_line_element2_t *mt;
    const char *ps = in_line, *pend = ps + in_len, *p, *p_s, *p_charset, *p_encode, *p_data, *ps_next;
    while ((ps < pend) && (pend - ps > 6)) {
        found = 0;
        p_s = ps;
        while ((p_s < pend) && (pend - p_s > 6)) {
            if (!(p_s = zmemmem(p_s, pend - p_s, "=?", 2))) {
                break;
            }
            if ((pend <= p_s) || (pend - p_s < 6)) {
                break;
            }
            p_charset = p_s + 2;

            if (!(p_encode = memchr(p_charset, '?', pend - p_charset))) {
                break;
            }
            clen = p_encode - p_charset;
            if (pend - p_encode < 3) {
                break;
            }
            p_encode++;
            encode = ztoupper(p_encode[0]);
            if (((encode!='B') && (encode != 'Q')) || (p_encode[1]!='?')) {
                p_s = p_encode - 1;
                continue;
            }
            p_data = p_encode + 2;
            if (pend <= p_data) {
                break;
            }
            found = 1;
            p = zmemmem(p_data, pend - p_data, "?=", 2);
            if (p) {
                ps_next = p + 2;
                dlen = p - p_data;
            } else {
                ps_next = pend;
                dlen = pend - p_data;
            }
            break;
        }
        if (!found) {
            break;
        }
        if (ps < p_s) {
            mt = mt_vec + count++;
            memset(mt, 0, sizeof(zmime_header_line_element2_t));
            mt->data = ps;
            mt->dlen = p_s - ps;
        }

        mt = mt_vec + count++;
        memset(mt, 0, sizeof(zmime_header_line_element2_t));
        mt->data = p_data;
        mt->dlen = dlen;
        mt->charset = p_charset;
        mt->clen = clen;
        mt->encode_type = encode;

        ps = ps_next;
    }
    if (ps < pend) {
        mt = mt_vec + count++;
        memset(mt, 0, sizeof(zmime_header_line_element2_t));
        mt->data = ps;
        mt->dlen = pend - ps;
    }

    return count;
}

const zvector_t *zmime_header_line_get_element_vector(const char *in_line, int in_len)
{
    if (in_len < 0) {
        in_len = strlen(in_line);
    }

    int i, mt_count = 0;
    const char *p = in_line;

    for (i = 0; i < in_len; i++, p++){
        if ((p[0] != '=') || (p[1] != '?')) {
            continue;
        }
        mt_count++;
        i++;
        p++;
    }
    mt_count = mt_count*2 + 10;
    zmime_header_line_element2_t *mt_vec = (zmime_header_line_element2_t *)zmalloc(mt_count * sizeof(zmime_header_line_element2_t));
    mt_count = zmime_header_line_get_element_vector_inner(in_line, in_len, mt_vec, mt_count);
    zvector_t *rv = zvector_create(mt_count + 1);
    for (i = 0 ; i < mt_count; i++) {
        zmime_header_line_element2_t *mt = mt_vec + i;
        zmime_header_line_element_t *element = zcalloc(1, sizeof(zmime_header_line_element_t));
        char *tmpp = (mt->clen?zmemdupnull(mt->charset, mt->clen):0);
        if (tmpp) {
            zstr_tolower(tmpp);
        }
        element->charset = tmpp;
        element->data = zmemdupnull(mt->data, mt->dlen);
        element->dlen = mt->dlen;
        element->encode_type = mt->encode_type;
        zvector_push(rv, element);
    }
    return rv;
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

/* }}} */

/* {{{ zmime_header_line_get_utf8 */
static void zmime_header_line_get_utf8_engine(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result, zbuf_t *bq_join, zbuf_t *out_string)
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

    for (i = 0; i < mt_count; i++) {
        mt = mt_vec + i;
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
        mtn = mt_vec + i + 1;
        while (1) {
            if (i + 1 >= mt_count) {
                break;
            }
            if (mtn->encode_type == 0) {
                int j;
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
            zbase64_decode(p, plen, out_string);
        } else if (mt->encode_type == 'Q') {
            zqp_decode_2047(p, plen, out_string);
        }
        ret = zbuf_len(out_string);

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
}

void zmime_header_line_get_utf8_inner(zmail_t *parser, const char *in_line, int in_len, zbuf_t *result)
{
    zbuf_t *bq_join = zmail_zbuf_cache_require(parser, -1);
    zbuf_t *out_string = zmail_zbuf_cache_require(parser, -1);
    zmime_header_line_get_utf8_engine(parser->src_charset_def, in_line, in_len, result, bq_join, out_string);
    zmail_zbuf_cache_release(parser, bq_join);
    zmail_zbuf_cache_release(parser, out_string);
}

void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result)
{
    zbuf_t *bq_join = zbuf_create(-1);
    zbuf_t *out_string = zbuf_create(-1);
    zmime_header_line_get_utf8_engine(src_charset_def, in_line, in_len, result, bq_join, out_string);
    zbuf_free(bq_join);
    zbuf_free(out_string);
}
/* }}} */

/*
vim600: fdm=marker
*/
