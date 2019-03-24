/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-15
 * ================================
 */

#include "zc.h"

void zmail_free_mime(zmail_t * parser, zmime_t * mime);
int zmail_decode_mime(zmail_t * parser, zmime_t * pmime, zmime_t * cmime, char *buf);
void zmime_header_decode_content_type(const char *data, size_t len , char **val, int *v_len , char **boundary, int *b_len , char **charset, int *c_len , char **name, int *n_len);

#define ___CASEEQ_LEN(a, b, n)          ((zchar_toupper(a[0]) == zchar_toupper((b)[0])) && (!strncasecmp(a,b,n)))
#define ___CASEEQ(a, b)                 ((zchar_toupper(a[0]) == zchar_toupper(b[0])) && (!strcasecmp(a,b)))
#define ___EQ_LEN(a, b, n)              ((a[0] == b[0]) && (!strncmp(a,b,n)))

#define ___mftell(parser) ((parser)->mail_pos - (parser)->mail_data)

/* ################################################################## */
static int ___get_header_line(zmail_t * parser, char **ptr)
{
    char *pbegin = parser->mail_pos;
    char *pend = parser->mail_data + parser->mail_size;
    char *ps, *p;
    int len = 0;

    *ptr = pbegin;
    if (pbegin > pend) {
        return 0;
    }
    if (pbegin[0] == '\n') {
        parser->mail_pos += 1;
        return 0;
    }
    if (pend > pbegin) {
        if ((pbegin[0] == '\r') && (pbegin[1] == '\n')) {
            parser->mail_pos += 2;
            return 0;
        }
    }

    ps = pbegin;
    while (pend > ps) {
        p = memchr(ps, '\n', pend - ps);
        if ((!p) || (p + 1 == pend)) {
            /* not found or to end */
            len = pend - pbegin;
            break;
        }
        if ((p[1] == ' ') || (p[1] == '\t')) {
            ps = p + 1;
            continue;
        }
        len = p - pbegin + 1;
        break;
    }

    parser->mail_pos += len;
    return len;
}

static inline int ___get_body_line(zmail_t * parser, char **ptr)
{
    char *pbegin = parser->mail_pos;
    char *pend = parser->mail_data + parser->mail_size;
    char *p;
    int len;

    *ptr = pbegin;

    len = pend - pbegin;
    if (len < 1) {
        return 0;
    }
    p = memchr(pbegin, '\n', len);

    if (p) {
        len = p - pbegin + 1;
        parser->mail_pos += len;
    } else {
        parser->mail_pos += len;
    }
    return len;
}

/* ################################################################## */
static int deal_content_type(zmail_t * parser, zmime_t * cmime, char *buf, int len)
{
    if (!ZEMPTY(cmime->type)) {
        return 0;
    }
    char *val, *boundary, *charset, *name;
    int v_len, b_len, c_len, n_len;

    zmime_header_decode_content_type(buf, len, &val, &v_len, &boundary, &b_len, &charset, &c_len, &name, &n_len);
    if (v_len) {
        cmime->type = zmpool_memdupnull(parser->mpool, val, v_len);
        ztolower(cmime->type);
    }
    if (b_len) {
        cmime->boundary = zmpool_memdupnull(parser->mpool, boundary, b_len);
    }
    if (c_len) {
        cmime->charset = zmpool_memdupnull(parser->mpool, charset, c_len);
    }
    if (n_len) {
        cmime->name = zmpool_memdupnull(parser->mpool, name, n_len);
    }

    return 0;
}

/* ################################################################## */
static int deal_single(zmail_t * parser, zmime_t * pmime, zmime_t * cmime, char *buf)
{
    int ret = 2, len, blen, tell;
    char *line;

    if (!pmime) {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    blen = (pmime->boundary?strlen(pmime->boundary):0);
    while (1) {
        tell = ___mftell(parser);
        if ((len = ___get_body_line(parser, &line)) < 1) {
            break;
        }
        if ((len < (blen + 2)) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }

        line += 2;
        if (!___CASEEQ_LEN(line, pmime->boundary, blen)) {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-')) {
            ret = 3;
        }
        cmime->body_len = tell - cmime->body_offset;
        break;
    }
    return (ret);
}

/* ################################################################## */
static int deal_multpart(zmail_t * parser, zmime_t * pmime, zmime_t * cmime, char *buf)
{
    int ret = 2;
    int len, blen;
    int have = 0;
    zmime_t *nmime = 0, *prev = 0;
    char *line;

    blen = (cmime->boundary?strlen(cmime->boundary):0);
    while (1) {
        //int offset_bak = ___mftell(parser);
        len = ___get_body_line(parser, &line);
        if (len < 1) {
            break;
        }
        have = 1;

        if (!cmime->boundary) {
            continue;
        }
        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!___CASEEQ_LEN(line, cmime->boundary, blen)) {
            continue;
        }
        //cmime->body_offset = offset_bak;
        while (1) {
            int used = 0;
            nmime = (zmime_t *) zmpool_calloc(parser->mpool, 1, sizeof(zmime_t));
            zvector_add(parser->all_mimes, nmime);
            ret = zmail_decode_mime(parser, cmime, nmime, buf);
            if (ret == 2 || ret == 3) {
                used = 1;
                nmime->parent = cmime;
                if (!prev) {
                    cmime->child = nmime;
                } else {
                    prev->next = nmime;
                }
                prev = nmime;
            }
            if (!used) {
                zvector_truncate(parser->all_mimes, ZVECTOR_LEN(parser->all_mimes)-1);
                zmail_free_mime(parser, nmime);
            }
            if (ret != 2) {
                break;
            }

        }
        break;
    }
    /* XXX */
    if (ret == 5)
        return (5);
    if (!have) {
        return (5);
    }
    ret = 2;
    if (!pmime) {
        return (ret);
    }

    blen = (pmime->boundary?strlen(pmime->boundary):0);
    len = 0;
    while ((len = ___get_body_line(parser, &line)) > 0) {
        if (!pmime->boundary) {
            continue;
        }
        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!___CASEEQ_LEN(line, pmime->boundary, blen)) {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-')) {
            ret = 3;
        }
        break;
    }
    cmime->body_len = ___mftell(parser) - cmime->body_offset - len;
    if (cmime->body_len < 0) {
        cmime->body_len = 0;
    }

    return (ret);
}

/* ################################################################## */
static int deal_message(zmail_t * parser, zmime_t * pmime, zmime_t * cmime, char *buf)
{
    return deal_single(parser, pmime, cmime, buf);

    int ret = 2,  decode = 1, len, blen, tell;
    char *line;

    if (!pmime) {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    if (!cmime->encoding) {
        decode = 0;
    } else if (___CASEEQ(cmime->encoding, "7bit")) {
        decode = 0;
    } else if (___CASEEQ(cmime->encoding, "8bit")) {
        decode = 0;
    } else if (___CASEEQ(cmime->encoding, "binary")) {
        decode = 0;
    }
    if (decode) {
        return deal_single(parser, pmime, cmime, buf);
    }

    blen = (pmime->boundary?strlen(pmime->boundary):0);
    while (1) {
        tell = ___mftell(parser);
        if ((len = ___get_body_line(parser, &line)) <= 0) {
            break;
        }
        tell = ___mftell(parser);

        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!___CASEEQ_LEN(line, pmime->boundary, blen)) {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-')) {
            ret = 3;
        }
        cmime->body_len = tell - cmime->body_offset;
        break;
    }
    return (ret);
}

/* ################################################################## */
#define ___clear_tail_rn(lll) { \
    int idx = cmime->header_offset + cmime->header_len; \
    if ((cmime->lll > 0) && (parser->mail_data[idx - 1] == '\n')) { idx--; cmime->lll--; } \
    if ((cmime->lll > 0) && (parser->mail_data[idx - 1] == '\r')) { cmime->lll--; } \
}

int zmail_decode_mime(zmail_t * parser, zmime_t * pmime, zmime_t * cmime, char *buf)
{
    char *line;
    int have_header = 0, ret = 0, llen, safe_llen;
    zsize_data_t *sd;

    cmime->parser = parser;

    cmime->type = zblank_buffer;
    cmime->boundary = zblank_buffer;
    cmime->charset = zblank_buffer;
    cmime->name = zblank_buffer;
    cmime->imap_section = zblank_buffer;

    cmime->header_offset = ___mftell(parser);
    zvector_reset(parser->tmp_header_lines);
    while (1) {
        safe_llen = llen = ___get_header_line(parser, &line);
        if (safe_llen > ZMAIL_HEADER_LINE_MAX_LENGTH) {
            safe_llen = ZMAIL_HEADER_LINE_MAX_LENGTH - 2;
        }
        if (llen == 0) {
            cmime->header_len = ___mftell(parser) - cmime->header_offset;
            ___clear_tail_rn(header_len);
            cmime->body_offset = ___mftell(parser);
            if (1) {
                /* re-save header to vector */
                cmime->header_lines = zvector_create(ZVECTOR_LEN(parser->tmp_header_lines));
                ZVECTOR_WALK_BEGIN(parser->tmp_header_lines, sd) {
                    zvector_add(cmime->header_lines, sd);
                } ZVECTOR_WALK_END;
            }
            break;
        }
        if (1) {
            /* save header to tmp vector*/
            sd = zmpool_malloc(parser->mpool, sizeof(zsize_data_t));
            sd->size = llen;
            sd->data = line;
            zvector_add(parser->tmp_header_lines, sd);
        }

        have_header = 1;
        if ((llen > 12) && (line[7]=='-') && (!strncasecmp(line, "Content-Type:", 13))) {

            int rlen = zmime_header_line_unescape_advanced(line, safe_llen, buf);
            buf[rlen] = 0;
            if (!strncasecmp(line, "Content-Type:", 13)){
                ret = deal_content_type(parser, cmime, buf + 13, rlen - 13);
            }
        }
        if (ret == 5) {
            return 5;
        }
    }

    /* deal mail body */
    if (cmime->type == 0 || *(cmime->type) == 0) {
        zmpool_free(parser->mpool, cmime->type);
        cmime->type = zmpool_strdup(parser->mpool, "text/plain");
    }
    if (!have_header) {
        return 4;
    }

    if (___CASEEQ_LEN(cmime->type, "multipart/", 10)) {
        int ppp = 1;
        zmime_t *parent;
        for (parent = cmime->parent; parent; parent = parent->parent) {
            ppp++;
        }
        if (ppp >= parser->mime_max_depth) {
            ret = deal_single(parser, pmime, cmime, buf);
        } else {
            ret = deal_multpart(parser, pmime, cmime, buf);
        }
    } else if (___CASEEQ_LEN(cmime->type, "message/", 8)) {
        ret = deal_message(parser, pmime, cmime, buf);
    } else {
        ret = deal_single(parser, pmime, cmime, buf);
    }

    if (cmime->body_len == 0) {
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
    }

    ___clear_tail_rn(body_len);

    return (ret);
}
