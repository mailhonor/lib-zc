/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "libzc.h"

#define ___CASEEQ_LEN(a, b, n)          ((zchar_toupper(a[0]) == zchar_toupper((b)[0])) && (!strncasecmp(a,b,n)))

#define ___MAX_2047_STRING_COUNT      127

typedef struct zmail_header_line_token_t zmail_header_line_token_t;
struct zmail_header_line_token_t {
    int encode;
    char charset[64];
    char *data;
    int len;
};

static int header_line_split(char *in_src, int in_len, zmail_header_line_token_t * mt_list)
{
    char *ps, *p1, *p2, *p3, *p, *pf, *pf_e, *pch, *pch_e, *pen, *pdata, *pdata_e;
    char *in_end = in_src + in_len;
    zmail_header_line_token_t *mt;
    int mt_count = 0;
    int tmp_len;

    if (in_len < 1) {
        return (-1);
    }

    mt = mt_list;
    ps = in_src;
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
            mt->len = pf_e - pf + 1;
            mt++;
            mt_count++;

            mt->encode = zchar_toupper(*pen);
            tmp_len = pch_e - pch + 1;
            if (tmp_len > 63) {
                tmp_len = 63;
            }
            zstrncpy(mt->charset, pch, tmp_len);
            mt->data = pdata;
            mt->len = pdata_e - pdata + 1;
            mt++;
            mt_count++;
            p = in_src;
            break;
        }
        if (!p) {
            mt->encode = 0;
            mt->len = strlen(ps);
            mt->data = ps;
            mt++;
            mt_count++;
            break;
        }
        if (mt_count >= ___MAX_2047_STRING_COUNT) {
            break;
        }
    }

    return mt_count;
}

int zmail_parser_header_value_decode(zmail_parser_t * parser, char *in_src, int in_len, char *out)
{
    /* Limit the length of the logic-line or the string within 10K */
    int ret, i;
    char out_string[ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16];
    int convert_len;
    zmail_header_line_token_t mt_list[___MAX_2047_STRING_COUNT + 1], *mt, *mtn;
    int mt_count;
    char *p;
    int plen;
    ZSTACK_BUF_FROM(result, out, ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16);
    ZSTACK_BUF(bq_join, ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16);

    *out = 0;
    if (in_len < 1) {
        return 0;
    }
    if (in_len > ZMAIL_HEADER_LINE_MAX_LENGTH) {
        in_len = ZMAIL_HEADER_LINE_MAX_LENGTH;
    }

    mt_count = 0;
    memset(mt_list, 0, sizeof(mt_list));
    mt_count = header_line_split(in_src, in_len, mt_list);

    for (i = 0; i < mt_count; i++) {
        mt = mt_list + i;
        if (mt->len == 0) {
            continue;
        }
        if ((mt->encode != 'B') && (mt->encode != 'Q')) {
            convert_len = zmail_parser_iconv(parser, 0, mt->data, mt->len, out_string, ZMAIL_HEADER_LINE_MAX_LENGTH * 3);
            if ((convert_len < 0) || (convert_len < 1)) {
                continue;
            }
            zbuf_memcat(result, out_string, convert_len);
            continue;
        }
        zbuf_memcpy(bq_join, mt->data, mt->len);
        mtn = mt + 1;
        while (1) {
            if (i + 1 >= mt_count) {
                break;
            }
            if (mtn->encode == 0) {
                int j;
                char c;
                for (j = 0; j < mtn->len; j++) {
                    c = mtn->data[j];
                    if (c == ' ') {
                        continue;
                    }
                    break;
                }
                if (j == mtn->len) {
                    i++;
                    mtn++;
                    continue;
                }
                break;
            }
            if ((mt->encode == mtn->encode) && (*(mt->charset)) && (*(mtn->charset)) && (!strcasecmp(mt->charset, mtn->charset))) {
                zbuf_memcat(bq_join, mtn->data, mtn->len);
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
        if (mt->encode == 'B') {
            ret = zbase64_decode(p, plen, p, ZMAIL_HEADER_LINE_MAX_LENGTH);
        } else if (mt->encode == 'Q') {
            ret = zqp_decode_2047(p, plen, p, ZMAIL_HEADER_LINE_MAX_LENGTH);
        }

        if (ret < 1) {
            continue;
        }
        convert_len = 0;
        {
            /* rfc 2231 */
            char *p = strchr(mt->charset, '*');
            if (p) {
                *p = 0;
            }
        }
        convert_len = zmail_parser_iconv(parser, mt->charset, p, ret, out_string, ZMAIL_HEADER_LINE_MAX_LENGTH * 3);
        if ((convert_len < 0) || (convert_len < 1)) {
            continue;
        }
        zbuf_memcat(result, out_string, convert_len);
    }
    for (i = 0; i < mt_count; i++) {
        mt = mt_list + i;
    }

    zbuf_terminate(result);
    ret = ZBUF_LEN(result);

    return ret;
}

int zmail_parser_header_value_decode_dup(zmail_parser_t * parser, char *in_src, int in_len, char **out_src)
{
    char out[ZMAIL_HEADER_LINE_MAX_LENGTH * 3 + 16];
    int out_len;
    char *trim;
    int trim_len;

    if (in_len > ZMAIL_HEADER_LINE_MAX_LENGTH) {
        in_len = ZMAIL_HEADER_LINE_MAX_LENGTH;
    }
    out_len = zmail_parser_header_value_decode(parser, in_src, in_len, out);

    trim_len = zmail_parser_header_value_trim(parser, out, out_len, &trim);
    if (trim_len < 1) {
        *out_src = zmpool_malloc(parser->mpool, 0);
        return 0;
    }

    *out_src = zmpool_memdup(parser->mpool, trim, trim_len);

    return trim_len;
}

int zmail_parser_header_value_dup(zmail_parser_t * parser, char *in_src, int in_len, char **out_src)
{
    in_len = zmail_parser_header_value_trim(parser, in_src, in_len, &in_src);
    if (in_len < 1) {
        *out_src = zmpool_memdup(parser->mpool, "", 0);
        return 0;
    }

    *out_src = zmpool_memdup(parser->mpool, in_src, in_len);

    return in_len;
}

/* ################################################################## */

int zmail_parser_mimetrim_dup(zmail_parser_t * parser, char *in_src, int in_len, char *out)
{
    char *p = out;
    char ch;
    int i;

    for (i = 0; i < in_len; i++) {
        ch = in_src[i];
        if ((ch == '\0') || (ch == '\r')) {
            continue;
        }
        if (ch != '\n') {
            *p++ = ch;
            if (p - out > ZMAIL_HEADER_LINE_MAX_LENGTH) {
                break;
            }
            continue;
        }
        i++;
        if (i == in_len) {
            break;
        }
        ch = in_src[i];
        if ((ch == ' ') || (ch == '\t')) {
            continue;
        }
        i--;
    }
    *p = 0;

    return (p - out);
}

/* ################################################################## */

int zmail_parser_header_value_trim(zmail_parser_t * parser, char *line, int len, char **ptr)
{
    char *ps;
    char *pend = line + len;
    int i;
    int ch;

    for (i = 0; i < len; i++) {
        ch = line[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n')) {
            continue;
        }
        break;
    }
    if (i == len) {
        return 0;
    }
    ps = line + i;
    len = pend - ps;
    for (i = len - 1; i >= 0; i--) {
        ch = ps[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n')) {
            continue;
        }
        break;
    }
    if (i < 0) {
        return 0;
    }

    *ptr = ps;

    return i + 1;
}

/* ################################################################## */
int zmail_parser_header_signle_token_decode_dup(zmail_parser_t * parser, char *line, int len, char **out_src)
{
    char *ps;
    char *pend = line + len;
    int i;
    int ch;

    for (i = 0; i < len; i++) {
        ch = line[i];
        if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch == '<')) {
            continue;
        }
        break;
    }
    if (i == len) {
        *out_src = zmpool_memdup(parser->mpool, "", 0);
        return 0;
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
        *out_src = zmpool_memdup(parser->mpool, "", 0);
        return 0;
    }

    *out_src = zmpool_memdup(parser->mpool, ps, i + 1);

    return i + 1;
}

/* ################################################################## */
int zmail_parser_save_header(zmail_parser_t * parser, zmail_mime_t * cmime, char *line, int len)
{
    zmail_header_line_t *header;
    char *p;

    header = zmpool_calloc(parser->mpool, 1, sizeof(zmail_header_line_t));
    if (cmime->header_head == 0) {
        cmime->header_head = cmime->header_tail = header;
    } else {
        cmime->header_tail->next = header;
        cmime->header_tail = header;
    }

    header->line = zmpool_memdup(parser->mpool, line, len);
    header->line_len = len;
    p = (char *)memchr(line, ':', len);
    if (!p) {
        header->name = zmpool_strdup(parser->mpool, "");
    } else {
        header->name = zmpool_memdup(parser->mpool, line, p - line);
    }

    return 0;
}

int zmail_parser_mail_header_decode_by_line(zmail_parser_t * parser, char *line, int len)
{
#define ___move(n)  {line+=n;len-=n;}
    if (___CASEEQ_LEN(line, "subject:", 8)) {
        if (!parser->subject) {
            ___move(8);
            int rlen = zmail_parser_header_value_dup(parser, line, len, &(parser->subject));
            if ((rlen > 0) && (!zmail_parser_only_test_parse)) {
                zmail_parser_header_value_decode_dup(parser, parser->subject, rlen, &(parser->subject_rd));
            }
        }
    } else if (___CASEEQ_LEN(line, "date:", 5)) {
        if (!parser->date) {
            ___move(5);
            zmail_parser_header_value_dup(parser, line, len, &(parser->date));
            parser->date_unix = zmail_parser_header_date_decode(parser, parser->date);
        }
    } else if (___CASEEQ_LEN(line, "from:", 5)) {
        if (!parser->from) {
            ___move(5);
            zmail_parser_addr_decode(parser, line, len, &(parser->from));
        }
    } else if (___CASEEQ_LEN(line, "to:", 3)) {
        if (!parser->to) {
            ___move(3);
            zmail_parser_addr_decode(parser, line, len, &(parser->to));
        }
    } else if (___CASEEQ_LEN(line, "cc:", 3)) {
        if (!parser->cc) {
            ___move(3);
            zmail_parser_addr_decode(parser, line, len, &(parser->cc));
        }
    } else if (___CASEEQ_LEN(line, "bcc:", 4)) {
        if (!parser->bcc) {
            ___move(4);
            zmail_parser_addr_decode(parser, line, len, &(parser->bcc));
        }
    } else if (___CASEEQ_LEN(line, "disposition-notification-to:", 28)) {
        if (!parser->receipt) {
            ___move(28);
            zmail_parser_addr_decode(parser, line, len, &(parser->receipt));
        }
    } else if (___CASEEQ_LEN(line, "message-id:", 11)) {
        if (!parser->message_id) {
            ___move(11);
            zmail_parser_header_signle_token_decode_dup(parser, line, len, &(parser->message_id));
        }
    } else if (___CASEEQ_LEN(line, "references:", 11)) {
        if (!zmail_parser_only_test_parse) {
            if (!parser->references) {
                ___move(11);
                zmail_parser_references_decode(parser, line, &(parser->references));
            }
        }
    } else if (___CASEEQ_LEN(line, "in-reply-to:", 12)) {
        if (!zmail_parser_only_test_parse) {
            if (!parser->in_reply_to) {
                ___move(12);
                zmail_parser_header_value_dup(parser, line, len, &(parser->in_reply_to));
            }
        }
    }

    return 0;
}

#undef ___CASEEQ_LEN
