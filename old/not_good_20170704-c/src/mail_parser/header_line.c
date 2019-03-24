/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "zc.h"

#define ___CASEEQ_LEN(a, b, n)          ((zchar_toupper(a[0]) == zchar_toupper((b)[0])) && (!strncasecmp(a,b,n)))

size_t zmime_header_line_element_split(const char *in_src, size_t in_len, zmime_header_line_element_t * mt_list, size_t mt_max_count)
{
    char *ps, *p1, *p2, *p3, *p, *pf, *pf_e, *pch, *pch_e, *pen, *pdata, *pdata_e;
    char *in_end = (char *)in_src + in_len;
    zmime_header_line_element_t *mt;
    int mt_count = 0;
    int tmp_len;

    mt = mt_list;
    ps = (char *)in_src;
    while (in_end - ps > 0) {
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
            mt->encode = 0;
            mt->data = pf;
            mt->size = pf_e - pf + 1;
            mt++;
            mt_count++;

            mt->encode = zchar_toupper(*pen);
            tmp_len = pch_e - pch + 1;
            if (tmp_len > 31) {
                tmp_len = 31;
            }
            zstrncpy(mt->charset, pch, tmp_len);
            {
                /* rfc 2231 */
                char *p = strchr(mt->charset, '*');
                if (p) {
                    *p = 0;
                }
            }
            mt->data = pdata;
            mt->size = pdata_e - pdata + 1;
            mt++;
            mt_count++;
            p = (char *)in_src;
            break;
        }
        if (!p) {
            mt->encode = 0;
            mt->size = strlen(ps);
            mt->data = ps;
            mt++;
            mt_count++;
            break;
        }
        if (mt_count >= mt_max_count) {
            break;
        }
    }

    return mt_count;
}

void zmime_header_line_get_utf8(const char *src_charset_def, const char *in_src, size_t in_len, zbuf_t *dest)
{
    int ret, i, convert_len, plen, mt_count;
    char *p;
    zmime_header_line_element_t mt_list[ZMAIL_HEADER_LINE_MAX_ELEMENT + 1], *mt, *mtn;
    ZSTACK_BUF(bq_join, ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16);
    ZSTACK_BUF(zout_string, ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16);

    zbuf_reset(dest);
    if (in_len > ZMAIL_HEADER_LINE_MAX_LENGTH) {
        in_len = ZMAIL_HEADER_LINE_MAX_LENGTH;
    }

    mt_count = 0;
    memset(mt_list, 0, sizeof(mt_list));
    mt_count = zmime_header_line_element_split(in_src, in_len, mt_list, ZMAIL_HEADER_LINE_MAX_ELEMENT);

    for (i = 0; i < mt_count; i++) {
        mt = mt_list + i;
        if (mt->size == 0) {
            continue;
        }
        if ((mt->encode != 'B') && (mt->encode != 'Q')) {
            zbuf_reset(zout_string);
            convert_len = zmime_iconv(src_charset_def, mt->data, mt->size, zout_string);
            if ((convert_len < 0) || (convert_len < 1)) {
                continue;
            }
            zbuf_memcat(dest, ZBUF_DATA(zout_string), convert_len);
            continue;
        }
        zbuf_memcpy(bq_join, mt->data, mt->size);
        mtn = mt + 1;
        while (1) {
            if (i + 1 >= mt_count) {
                break;
            }
            if (mtn->encode == 0) {
                int j;
                char c;
                for (j = 0; j < mtn->size; j++) {
                    c = mtn->data[j];
                    if (c == ' ') {
                        continue;
                    }
                    break;
                }
                if (j == mtn->size) {
                    i++;
                    mtn++;
                    continue;
                }
                break;
            }
            if ((mt->encode == mtn->encode) && (*(mt->charset)) && (*(mtn->charset)) && (!strcasecmp(mt->charset, mtn->charset))) {
                zbuf_memcat(bq_join, mtn->data, mtn->size);
                i++;
                mtn++;
                continue;
            }
            break;
        }
        p = ZBUF_DATA(bq_join);
        plen = ZBUF_LEN(bq_join);
        p[plen] = 0;
        ret = 0;
        zbuf_reset(zout_string);
        if (mt->encode == 'B') {
            ret = zbase64_decode_zbuf(p, plen, zout_string);
        } else if (mt->encode == 'Q') {
            ret = zqp_decode_2047_zbuf(p, plen, zout_string);
        }

        if (ret < 1) {
            continue;
        }
        zbuf_memcpy(bq_join, ZBUF_DATA(zout_string), ZBUF_LEN(zout_string));
        convert_len = 0;
        zbuf_reset(zout_string);
        convert_len = zmime_iconv(mt->charset, ZBUF_DATA(bq_join), ZBUF_LEN(bq_join), zout_string);
        if ((convert_len < 0) || (convert_len < 1)) {
            continue;
        }
        zbuf_memcat(dest, ZBUF_DATA(zout_string), convert_len);
    }
    zbuf_terminate(dest);
}

size_t zmime_header_line_unescape_advanced(const char *data, size_t size, char *dest)
{
    char ch, *src = (char *)(data);
    size_t i, rlen = 0;

    if (size > ZMAIL_HEADER_LINE_MAX_LENGTH){
        size = ZMAIL_HEADER_LINE_MAX_LENGTH;
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

size_t zmime_header_line_unescape(const char *data, size_t size, char *dest, ssize_t dest_size)
{
    char ch, *src = (char *)(data);
    size_t i, rlen = 0;

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
            Z_DF_ADD_CHAR(dest_size, dest, rlen, '\n');
        }
        Z_DF_ADD_CHAR(dest_size, dest, rlen, ch);
        if ((dest_size > 0) && (rlen >= dest_size)) {
            break;
        }
    }

    return rlen;
}

void zmime_header_get_first_token(const char *line, size_t len, char **val, int *vlen)
{
    char *ps, *pend = (char *)line + len;
    int i;
    int ch;

    *val = (char *)line;
    *vlen = 0;
    for (i = 0; i < len; i++) {
        ch = line[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '<')) {
            continue;
        }
        break;
    }
    if (i == len) {
        return;
    }
    ps = (char *)line + i;
    len = pend - ps;
    for (i = len - 1; i >= 0; i--) {
        ch = ps[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '>')) {
            continue;
        }
        break;
    }
    if (i < 0) {
        return;
    }

    *val = ps;
    *vlen = i + 1;
    return;
}
#undef ___CASEEQ_LEN
