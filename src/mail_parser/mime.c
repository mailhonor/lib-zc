/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-15
 * ================================
 */

#include "libzc.h"

#define ___CASEEQ_LEN(a, b, n)          ((zchar_toupper(a[0]) == zchar_toupper((b)[0])) && (!strncasecmp(a,b,n)))
#define ___CASEEQ(a, b)                 ((zchar_toupper(a[0]) == zchar_toupper(b[0])) && (!strcasecmp(a,b)))
#define ___EQ_LEN(a, b, n)              ((a[0] == b[0]) && (!strncmp(a,b,n)))

#define ___mftell(parser) ((parser)->mail_pos - (parser)->mail_data)

/* ################################################################## */
static inline int ___zmail_parser_get_body_line(zmail_parser_t * parser, char **ptr)
{
    char *pbegin = parser->mail_pos;
    char *pend = parser->mail_data + parser->mail_size;
    char *p;
    int len;

    *ptr = pbegin;

    len = pend - pbegin;
    if (len < 1)
    {
        return 0;
    }
    p = memchr(pbegin, '\n', len);

    if (p)
    {
        len = p - pbegin + 1;
        parser->mail_pos += len;
    }
    else
    {
        parser->mail_pos += len;
    }
    return len;
}

/* ################################################################## */
static void mime_free_one(zmail_parser_t * parser, zmail_mime_t * mime)
{
#define ___ha(a)	{if(mime->a) zmpool_free(parser->mpool, mime->a);}
    ___ha(type);
    ___ha(encoding);
    ___ha(charset);
    ___ha(disposition);
    ___ha(boundary);
    ___ha(name);
    ___ha(filename);
    ___ha(filename_star);
    ___ha(name_rd);
    ___ha(filename_rd);
    ___ha(content_id);
#undef ___ha

    /* header */
    zmail_header_line_t *header, *hnext;
    for (header = mime->header_head; header; header = hnext)
    {
        hnext = header->next;
        zmpool_free(parser->mpool, header->name);
        zmpool_free(parser->mpool, header->line);
        zmpool_free(parser->mpool, header);
    }

    /* self */
    zmpool_free(parser->mpool, mime);
}

void zmail_parser_free_mime(zmail_parser_t * parser, zmail_mime_t * mime)
{
    zmail_mime_t *next;

    for (; mime; mime = next)
    {
        next = mime->all_next;
        mime_free_one(parser, mime);
    }
}

/* ################################################################## */
static void mime_format_one(zmail_parser_t * parser, zmail_mime_t * mime)
{
#define ___ha(a)	{if(!(mime->a)) mime->a = zmpool_malloc(parser->mpool, 0);}
    ___ha(type);
    ___ha(encoding);
    ___ha(charset);
    ___ha(disposition);
    ___ha(boundary);
    ___ha(name);
    ___ha(filename);
    ___ha(filename_star);
    ___ha(name_rd);
    ___ha(filename_rd);
    ___ha(content_id);
#undef ___ha
}

void zmail_parser_format_mime(zmail_parser_t * parser, zmail_mime_t * mime)
{
    zmail_mime_t *next;

    for (; mime; mime = next)
    {
        next = mime->all_next;
        mime_format_one(parser, mime);
    }
}

/* ################################################################## */
static int deal_content_type(zmail_parser_t * parser, zmail_mime_t * cmime, char *buf, int len)
{
    if (cmime->type)
    {
        return 0;
    }
    zmail_parser_header_parse_param(parser, cmime, buf, len, &(cmime->type));
    if (cmime->type)
    {
        ztolower(cmime->type);
    }
    return 0;
}

static int deal_content_transfer_encoding(zmail_parser_t * parser, zmail_mime_t * cmime, char *buf, int len)
{
    if (cmime->encoding)
    {
        return 0;
    }
    zmail_parser_header_parse_param(parser, cmime, buf, len, &(cmime->encoding));
    return 0;
}

static int deal_content_disposition(zmail_parser_t * parser, zmail_mime_t * cmime, char *buf, int len)
{
    if (cmime->disposition)
    {
        return 0;
    }
    zmail_parser_header_parse_param(parser, cmime, buf, len, &(cmime->disposition));
    return 0;
}

/* ################################################################## */
static int deal_single(zmail_parser_t * parser, zmail_mime_t * pmime, zmail_mime_t * cmime, char *buf)
{
    int ret = 2;
    int len, blen;
    int tell;
    char *line;

    if (!pmime)
    {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    blen = pmime->boundary_len;
    while (1)
    {
        tell = ___mftell(parser);
        if ((len = ___zmail_parser_get_body_line(parser, &line)) < 1)
        {
            break;
        }
        if ((len < blen) || (line[0] != '-') || (line[1] != '-'))
        {
            continue;
        }
        if (!___CASEEQ_LEN(line, pmime->boundary, blen))
        {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-'))
        {
            ret = 3;
        }
        cmime->body_len = tell - cmime->body_offset;
        break;
    }
    return (ret);
}

/* ################################################################## */
static int deal_multpart(zmail_parser_t * parser, zmail_mime_t * pmime, zmail_mime_t * cmime, char *buf)
{
    int ret = 2;
    int len, blen;
    int have = 0;
    zmail_mime_t *nmime = 0, *prev = 0;
    char *line;

    blen = cmime->boundary_len;
    while ((len = ___zmail_parser_get_body_line(parser, &line)) > 0)
    {
        have = 1;

        if (!cmime->boundary)
        {
            continue;
        }
        if ((len < blen) || (line[0] != '-') || (line[1] != '-'))
        {
            continue;
        }
        if (!___CASEEQ_LEN(line, cmime->boundary, blen))
        {
            continue;
        }
        while (1)
        {
            int used = 0;
            nmime = (zmail_mime_t *) zmpool_calloc(parser->mpool, 1, sizeof(zmail_mime_t));
            ret = zmail_parser_decode_mime(parser, cmime, nmime, buf);
            if (ret == 2 || ret == 3)
            {
                used = 1;
                nmime->parent = cmime;
                if (!prev)
                {
                    cmime->child = nmime;
                }
                else
                {
                    prev->next = nmime;
                }
                prev = nmime;
            }
            if (!used)
            {
                zmail_parser_free_mime(parser, nmime);
            }
            if (ret != 2)
            {
                break;
            }

        }
        break;
    }
    /* XXX */
    if (ret == 5)
        return (5);
    if (!have)
    {
        return (5);
    }
    ret = 2;
    if (!pmime)
    {
        return (ret);
    }

    blen = pmime->boundary_len;
    while ((len = ___zmail_parser_get_body_line(parser, &line)) > 0)
    {
        if (!pmime->boundary)
        {
            continue;
        }
        if ((len < blen) || (line[0] != '-') || (line[1] != '-'))
        {
            continue;
        }
        if (!___CASEEQ_LEN(line, pmime->boundary, blen))
        {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-'))
        {
            ret = 3;
        }
        break;
    }
    return (ret);
}

/* ################################################################## */
static int deal_message(zmail_parser_t * parser, zmail_mime_t * pmime, zmail_mime_t * cmime, char *buf)
{
    int ret = 2;
    int decode = 1;
    int len, blen;
    long tell;
    char *line;

    if (!pmime)
    {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    if (!cmime->encoding)
    {
        decode = 0;
    }
    else if (___CASEEQ(cmime->encoding, "7BIT"))
    {
        decode = 0;
    }
    else if (___CASEEQ(cmime->encoding, "8BIT"))
    {
        decode = 0;
    }
    else if (___CASEEQ(cmime->encoding, "BINARY"))
    {
        decode = 0;
    }
    if (decode)
    {
        return deal_single(parser, pmime, cmime, buf);
    }

    blen = pmime->boundary_len;
    while (1)
    {
        tell = ___mftell(parser);
        if ((len = ___zmail_parser_get_body_line(parser, &line)) <= 0)
        {
            break;
        }
        tell = ___mftell(parser);

        if ((len < blen) || (line[0] != '-') || (line[1] != '-'))
        {
            continue;
        }
        if (!___CASEEQ_LEN(line, pmime->boundary, blen))
        {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-'))
        {
            ret = 3;
        }
        cmime->body_len = tell - cmime->body_offset;
        break;
    }
    return (ret);
}

/* ################################################################## */
int zmail_parser_decode_mime(zmail_parser_t * parser, zmail_mime_t * pmime, zmail_mime_t * cmime, char *buf)
{
    char *line;
    int have = 0;
    int ret = 0;
    int idx;
    int hlen, len;

    cmime->header_offset = ___mftell(parser);
    while (1)
    {
        hlen = zmail_parser_get_header_line(parser, &line);
        len = zmail_parser_mimetrim_dup(parser, line, hlen, buf);
        if (len < 1)
        {
            cmime->header_len = ___mftell(parser) - cmime->header_offset;
            idx = cmime->header_offset + cmime->header_len;
            if ((cmime->header_len > 0) && (parser->mail_data[idx - 1] == '\n'))
            {
                idx--;
                cmime->header_len--;
            }
            if ((cmime->header_len > 0) && (parser->mail_data[idx - 1] == '\r'))
            {
                cmime->header_len--;
            }
            if (!pmime)
            {
                parser->header_len = cmime->header_len;
            }
            cmime->body_offset = ___mftell(parser);
            break;
        }
        zmail_parser_save_header(parser, cmime, line, hlen);

        buf[len] = 0;
        have = 1;
        if (pmime == 0)
        {
            zmail_parser_mail_header_decode_by_line(parser, buf, len);
        }
        if (___CASEEQ_LEN(buf, "Content-Type:", 13))
        {
            ret = deal_content_type(parser, cmime, buf + 13, len - 13);
        }
        else if (___CASEEQ_LEN(buf, "Content-Transfer-Encoding:", 26))
        {
            ret = deal_content_transfer_encoding(parser, cmime, buf + 26, len - 26);
        }
        else if (___CASEEQ_LEN(buf, "Content-Disposition:", 20))
        {
            ret = deal_content_disposition(parser, cmime, buf + 20, len - 20);
        }
        else if (___CASEEQ_LEN(buf, "Content-ID:", 11))
        {
            if (!cmime->content_id)
            {
                zmail_parser_header_signle_token_decode_dup(parser, buf + 11, len - 11, &(cmime->content_id));
            }
        }
        if (ret == 5)
            return 5;
    }

    /* deal mail body */
    if (cmime->type == 0 || *(cmime->type) == 0)
    {
        zmpool_free(parser->mpool, cmime->type);
        cmime->type = zmpool_strdup(parser->mpool, "text/plain");
    }
    if (!have)
    {
        return 4;
    }

    if (___CASEEQ_LEN(cmime->type, "multipart/", 10))
    {
        int ppp = 1;
        zmail_mime_t *parent;
        for (parent = cmime->parent; parent; parent = parent->parent)
        {
            ppp++;
        }
        if (ppp >= parser->mime_max_depth)
        {
            ret = deal_single(parser, pmime, cmime, buf);
        }
        else
        {
            ret = deal_multpart(parser, pmime, cmime, buf);
        }
    }
    else if (___CASEEQ_LEN(cmime->type, "message/", 8))
    {
        ret = deal_message(parser, pmime, cmime, buf);
    }
    else
    {
        ret = deal_single(parser, pmime, cmime, buf);
    }

    if (cmime->body_len == 0)
    {
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
    }

    idx = cmime->body_offset + cmime->body_len;
    if ((cmime->body_len > 0) && (parser->mail_data[idx - 1] == '\n'))
    {
        idx--;
        cmime->body_len--;
    }
    if ((cmime->body_len > 0) && (parser->mail_data[idx - 1] == '\r'))
    {
        cmime->body_len--;
    }

    return (ret);
}
