/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-10
 * ================================
 */

#include "libzc.h"

int zmail_parser_only_test_parse = 0;

static void format_parser(zmail_parser_t * parser)
{
    zmpool_t *imp = parser->mpool;

#define ___fh(a)	{if(!(parser->a)) parser->a = zmpool_malloc(imp, 0);}
    ___fh(subject);
    ___fh(subject_rd);
    ___fh(date);
    ___fh(in_reply_to);
    ___fh(message_id);
#undef ___fh

    zmail_parser_format_mime(parser, parser->mime);
}

/* ################################################################## */
static void set_mime_child_chain(zmail_mime_t * mime, zmail_mime_t ** last)
{
    zmail_mime_t *m, *next;

    for (m = mime->child; m; m = next) {
        (*last)->all_next = m;
        *last = m;
        next = m->next;
        set_mime_child_chain(m, last);
    }

}

static void set_mime_chain(zmail_mime_t * mime)
{
    zmail_mime_t *next, *last;

    if (!mime) {
        return;
    }
    last = 0;
    for (; mime; mime = next) {
        if (last == 0) {
            last = mime;
        } else {
            last->all_next = mime;
            last = mime;
        }
        next = mime->next;
        set_mime_child_chain(mime, &last);
    }
}

/* ################################################################## */

zmail_parser_t *zmail_parser_create_mpool(zmpool_t * imp, char *mail_data, int mail_data_len)
{
    zmail_parser_t *parser;

    parser = (zmail_parser_t *) zmpool_calloc(imp, 1, sizeof(zmail_parser_t));
    parser->mpool = imp;

    parser->mail_data = mail_data;
    parser->mail_pos = mail_data;
    parser->mail_size = mail_data_len;

    zmail_parser_set_default_charset(parser, "GB18030", "UTF-8");
    parser->mime_max_depth = 5;

    return parser;
}

int zmail_parser_run(zmail_parser_t * parser)
{
    char buf[ZMAIL_HEADER_LINE_MAX_LENGTH + 16];

    parser->mime = (zmail_mime_t *) zmpool_calloc(parser->mpool, 1, sizeof(zmail_mime_t));
    zmail_parser_decode_mime(parser, 0, parser->mime, buf);
    set_mime_chain(parser->mime);
    format_parser(parser);

    zmail_parser_mime_identify_type(parser);

    return 0;
}

int zmail_parser_set_default_charset(zmail_parser_t * parser, char *default_src_charset, char *default_dest_charset)
{
    if (!ZEMPTY(default_src_charset)) {
        strncpy(parser->default_src_charset, default_src_charset, 31);
    }
    if (!ZEMPTY(default_dest_charset)) {
        strncpy(parser->default_dest_charset, default_dest_charset, 31);
    }

    return 0;
}

int zmail_parser_set_mime_max_depth(zmail_parser_t * parser, int length)
{
    parser->mime_max_depth = length;

    return 0;
}

/* ################################################################## */

zmail_parser_t *zmail_parser_create(char *mail_data, int mail_data_len)
{
    return zmail_parser_create_mpool(0, mail_data, mail_data_len);
}

void zmail_parser_free(zmail_parser_t * parser)
{
    zmpool_t *imp = parser->mpool;

#define ___fh(a)	{if(parser->a) zmpool_free(imp, parser->a);}
#define ___ft(a)	{if(parser->a) zmail_parser_addr_free(parser, parser->a);}
    ___fh(subject);
    ___fh(subject_rd);
    ___fh(date);
    ___ft(from);
    ___ft(sender);
    ___ft(reply_to);
    ___ft(to);
    ___ft(cc);
    ___ft(bcc);
    ___fh(in_reply_to);
    ___fh(message_id);
    ___ft(receipt);
#undef ___fh
#undef ___ft

    /* references */
    zmail_references_t *refs, *refs_next;
    for (refs = parser->references; refs; refs = refs_next) {
        refs_next = refs->next;
        zmpool_free(imp, refs->message_id);
        zmpool_free(imp, refs);
    }

    /* mime-tree */
    zmail_parser_free_mime(parser, parser->mime);
    zmpool_free(parser->mpool, parser->text_mime_list);
    zmpool_free(parser->mpool, parser->view_mime_list);
    zmpool_free(parser->mpool, parser->attachment_mime_list);

    /* self */
    zmpool_free(parser->mpool, parser);
}

/* ################################################################## */
int zmail_parser_decode_mime_body_to_df(zmail_parser_t * parser, zmail_mime_t * mime, void *filter, int filter_type)
{
    int bq = 0, ret;
    char *in_src = parser->mail_data + mime->body_offset;
    int in_len = mime->body_len;
    char *encoding = mime->encoding;
    int tlen;

    if (!encoding || !*encoding) {
        bq = 0;
    } else if (!strcasecmp(encoding, "base64")) {
        bq = 'b';
    } else if (!strcasecmp(encoding, "quoted-printable")) {
        bq = 'q';
    } else {
        bq = 0;
    }

    if (!bq) {
        if (filter_type > 0) {
            tlen = (filter_type > in_len) ? in_len : filter_type;
            memcpy(filter, in_src, tlen);
            ((char *)filter)[tlen] = 0;
            return tlen;
        } else {
            zdata_filter_write(filter, filter_type, in_src, in_len);
            return in_len;
        }
    }

    if (bq == 'b') {
        ret = zbase64_decode_to_df(in_src, in_len, filter, filter_type);
    } else {
        ret = zqp_decode_2045_to_df(in_src, in_len, filter, filter_type);
    }

    return ret;
}

int zmail_parser_decode_text_mime_body_to_df(zmail_parser_t * parser, zmail_mime_t * mime, void *filter, int filter_type)
{
    int bq = 0, convert_len;
    char *in_src = parser->mail_data + mime->body_offset;
    int in_len = mime->body_len;
    char *encoding = mime->encoding;
    char *buf3;
    int buf3_flag = 0, buf3_len;
    char buf3_stack[1024 * 64 + 16];
    int tlen;

    if (!encoding || !*encoding) {
        bq = 0;
    } else if (!strcasecmp(encoding, "base64")) {
        bq = 'b';
    } else if (!strcasecmp(encoding, "quoted-printable")) {
        bq = 'q';
    } else {
        bq = 0;
    }

    if (bq) {
        if (in_len > 1024 * 64) {
            buf3 = (char *)zmalloc(in_len + 16);
            buf3_flag = 1;
        } else {
            buf3 = buf3_stack;
        }
        if (bq == 'b') {
            buf3_len = zbase64_decode(in_src, in_len, buf3, in_len + 8);
        } else {
            buf3_len = zqp_decode_2045(in_src, in_len, buf3, in_len + 8);
        }
        buf3[buf3_len] = 0;
    } else {
        buf3 = in_src;
        buf3_len = in_len;
    }

    convert_len = zmail_parser_iconv(parser, mime->charset, buf3, buf3_len, filter, filter_type);
    if (convert_len < 0) {
        if (filter_type > 0) {
            tlen = (filter_type > buf3_len) ? buf3_len : filter_type;
            memcpy(filter, buf3, tlen);
            ((char *)filter)[tlen] = 0;
        } else {
            tlen = buf3_len;
            zdata_filter_write(filter, filter_type, buf3, buf3_len);
        }
        if (buf3_flag) {
            zfree(buf3);
        }
        return tlen;
    }
    if (buf3_flag) {
        zfree(buf3);
    }
    if (filter_type > 0) {
        ((char *)filter)[convert_len] = 0;
    }

    return convert_len;
}
