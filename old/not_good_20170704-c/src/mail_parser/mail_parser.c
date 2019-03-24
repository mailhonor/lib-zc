/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-10
 * ================================
 */

#include "zc.h"

#define ___CASEEQ_LEN(a, b, n)          ((zchar_toupper(a[0]) == zchar_toupper((b)[0])) && (!strncasecmp(a,b,n)))
int zmail_decode_mime(zmail_t * parser, zmime_t * pmime, zmime_t * cmime, char *buf);
void zmime_header_decode_content_disposition(const char *data, size_t len , char **val, int *v_len , char **filename, int *f_len , zbuf_t *filename_2231 , int *filename_2231_with_charset);
void zmime_header_decode_content_transfer_encoding(const char *data, size_t len, char **val, int *v_len);
void zmail_set_imap_section(zmail_t * parser);
void zmail_mime_classify(zmail_t * parser);


/* ################################################################## */
void zmail_free_mime(zmail_t * parser, zmime_t * mime)
{
#define ___ha(a)	{if(mime->a) zmpool_free(parser->mpool, mime->a);}
    ___ha(type);
    ___ha(encoding);
    ___ha(charset);
    ___ha(disposition);
    ___ha(boundary);
    ___ha(name);
    ___ha(filename);
    ___ha(filename2231);
    ___ha(name_utf8);
    ___ha(filename_utf8);
    ___ha(content_id);
    ___ha(imap_section);
#undef ___ha

    /* header */
    zsize_data_t *sd;
    ZVECTOR_WALK_BEGIN(mime->header_lines, sd) {
        zmpool_free(parser->mpool, sd);
    } ZVECTOR_WALK_END;
    zvector_free(mime->header_lines);

    /* self */
    zmpool_free(parser->mpool, mime);
}

/* ################################################################## */
zmail_t *zmail_parser_create_MPOOL(zmpool_t * imp, const char *mail_data, size_t mail_data_len)
{
    zmail_t *parser;

    parser = (zmail_t *) zmpool_calloc(imp, 1, sizeof(zmail_t));
    parser->mpool = imp;

    parser->mail_data = (char *)mail_data;
    parser->mail_pos = (char *)mail_data;
    parser->mail_size = (int)mail_data_len;

    parser->mime_max_depth = 5;

    parser->top_mime = (zmime_t *) zmpool_calloc(parser->mpool, 1, sizeof(zmime_t));
    parser->all_mimes = zvector_create(256);
    zvector_add(parser->all_mimes, parser->top_mime);

    parser->tmp_header_lines = zvector_create(128);

    return parser;
}

void zmail_parser_option_mime_max_depth(zmail_t * parser, int length)
{
    parser->mime_max_depth = length;
}

void zmail_parser_option_src_charset_def(zmail_t * parser, const char *src_charset_def)
{
    if (src_charset_def) {
        snprintf(parser->src_charset_def, 31, "%s", src_charset_def);
    }
}

void zmail_parser_run(zmail_t * parser)
{
    char buf[ZMAIL_HEADER_LINE_MAX_LENGTH + 16];
    zmail_decode_mime(parser, 0, parser->top_mime, buf);
}

/* ################################################################## */

zmail_t *zmail_parser_create(const char *mail_data, int mail_data_len)
{
    return zmail_parser_create_MPOOL(0, mail_data, mail_data_len);
}

void zmail_parser_free(zmail_t * parser)
{
    zmpool_t *imp = parser->mpool;

#define ___fh(a)	{if(parser->a) zmpool_free(imp, parser->a);}
#define ___ft(a)	{if(parser->a)  { zmpool_free(imp, parser->a->name); zmpool_free(imp, parser->a->name_utf8); \
        zmpool_free(imp, parser->a->address); zmpool_free(imp, parser->a); }}
#define ___ftv(a)	{if(parser->a) zmime_address_vector_free(parser->a);}
    ___fh(subject);
    ___fh(subject_utf8);
    ___fh(date);
    ___ft(from);
    ___ft(sender);
    ___ft(reply_to);
    ___ftv(to);
    ___ftv(cc);
    ___ftv(bcc);
    ___fh(in_reply_to);
    ___fh(message_id);
    ___ft(receipt);
#undef ___fh
#undef ___ft
#undef ___ftv

    /* references */
    if (parser->references) {
        char *ref;
        ZVECTOR_WALK_BEGIN(parser->references, ref) {
            zmpool_free(imp, ref);
        } ZVECTOR_WALK_END;
        zvector_free(parser->references);
    }

    /* mime */
    zmime_t *mime;
    ZVECTOR_WALK_BEGIN(parser->all_mimes, mime) {
        zmail_free_mime(parser, mime);
    } ZVECTOR_WALK_END;
    if (parser->all_mimes) {
        zvector_free(parser->all_mimes);
    }
    if (parser->text_mimes) {
        zvector_free(parser->text_mimes);
    }
    if (parser->show_mimes) {
        zvector_free(parser->show_mimes);
    }
    if (parser->attachment_mimes) {
        zvector_free(parser->attachment_mimes);
    }

    /* tmp */
    zvector_free(parser->tmp_header_lines);

    /* self */
    zmpool_free(parser->mpool, parser);
}


/* ################################################################## */
char *zmime_get(zmime_t *mime, int module)
{
    zmail_t *parser = mime->parser;
    char buf[ZMAIL_HEADER_LINE_MAX_LENGTH + 10], *val;
    int blen, vlen;
    ZSTACK_BUF_FROM(zb, buf, ZMAIL_HEADER_LINE_MAX_LENGTH);
    ZSTACK_BUF(zb2, 1024 * 100);
    zsize_data_t *sd;

    if (module == 1) {
        if (!mime->encoding) {
            mime->encoding = zblank_buffer;
            ZVECTOR_WALK_BEGIN(mime->header_lines, sd) {
                if ((sd->size>25) && (!strncasecmp(sd->data, "Content-Transfer-Encoding:", 26))) {
                    blen =zmime_header_line_unescape_advanced(sd->data + 26, sd->size - 26, buf);
                    buf[blen] = 0;
                    zmime_header_decode_content_transfer_encoding(buf, blen, &val, &vlen);
                    if (vlen > 0) {
                        mime->encoding = zmpool_memdupnull(parser->mpool, val, vlen);
                        ztolower(mime->encoding);
                    }
                    break;
                }
            } ZVECTOR_WALK_END;
        }
        return mime->encoding;
    } else if ((module == 2) || (module == 4) || (module == 5) || (module == 6)) {
        if (!mime->disposition) {
            char *fn; int fnl;
            mime->disposition = zblank_buffer;
            mime->filename = zblank_buffer;
            mime->filename2231 = zblank_buffer;
            ZVECTOR_WALK_BEGIN(mime->header_lines, sd) {
                if ((sd->size>19) && (!strncasecmp(sd->data, "Content-Disposition:", 20))) {
                    blen =zmime_header_line_unescape_advanced(sd->data + 20, sd->size - 20, buf);
                    buf[blen] = 0;
                    int with_charset;
                    zmime_header_decode_content_disposition(buf, blen, &val, &vlen, &fn, &fnl, zb2, &with_charset);
                    if (vlen > 0) {
                        mime->disposition = zmpool_memdupnull(parser->mpool, val, vlen);
                        ztolower(mime->disposition);
                    }
                    if (fnl > 0) {
                        mime->filename = zmpool_memdupnull(parser->mpool, fn, fnl);
                    }
                    if (ZBUF_LEN(zb2) > 0) {
                        mime->filename2231 = zmpool_memdupnull(parser->mpool, ZBUF_DATA(zb2), ZBUF_LEN(zb2));
                        mime->filename2231_with_charset = with_charset;
                    }
                    break;
                }
            } ZVECTOR_WALK_END;
        }
        if ((module == 6) && (!mime->filename_utf8)) {
            mime->filename_utf8 = zblank_buffer;
            if (*(mime->filename2231)) {
                zbuf_reset(zb2);
                zmime_header_line_2231_get_utf8(parser->src_charset_def, mime->filename2231, strlen(mime->filename2231), mime->filename2231_with_charset, zb2);
                mime->filename_utf8 = zmpool_memdupnull(parser->mpool, ZBUF_DATA(zb2), ZBUF_LEN(zb2));
            }
            if ((!*(mime->filename_utf8)) && (*(mime->filename))) {
                zbuf_reset(zb2);
                zmime_header_line_get_utf8(parser->src_charset_def, mime->filename, strlen(mime->filename), zb2);
                mime->filename_utf8 = zmpool_memdupnull(parser->mpool, ZBUF_DATA(zb2), ZBUF_LEN(zb2));
            }
        }
        if (module == 2) {
            return mime->disposition;
        } else if (module == 4) {
            return mime->filename;
        } else if (module == 5) {
            return mime->filename2231;
        } else if (module == 6) {
            return mime->filename_utf8;
        }
    } else if (module == 3) {
        if (!mime->name_utf8) {
            mime->name_utf8 = zblank_buffer;
            zmime_header_line_get_utf8(parser->src_charset_def, mime->name, strlen(mime->name), zb2);
            mime->name_utf8 = zmpool_memdupnull(parser->mpool, ZBUF_DATA(zb2), ZBUF_LEN(zb2));
        }
        return mime->name_utf8;
    } else if (module == 7) {
        if (!mime->content_id) {
            mime->content_id = zblank_buffer;
            ZVECTOR_WALK_BEGIN(mime->header_lines, sd) {
                if ((sd->size>10) && (!strncasecmp(sd->data, "Content-ID:", 11))) {
                    blen =zmime_header_line_unescape_advanced(sd->data + 11, sd->size - 11, buf);
                    buf[blen] = 0;
                    zmime_header_get_first_token(buf, blen, &val, &vlen);
                    if (vlen > 0) {
                        mime->content_id = zmpool_memdupnull(parser->mpool, val, vlen);
                    }
                    break;
                }
            } ZVECTOR_WALK_END;
        }
        return mime->content_id;
    }

    return zblank_buffer;
}

/* sn == 0: first, sn == -1: last */
const zsize_data_t *zmime_header_line(zmime_t *m, const char *header_name, int sn);
void zmime_header_lines(zmime_t *m, const char *header_name, zvector_t *vec);
ssize_t zmime_decoded_content(zmime_t *m, zbuf_t *dest);
ssize_t zmime_decoded_content_utf8(zmime_t *m, zbuf_t *dest)
{
    zmail_t *parser = m->parser;
    int bq = 0, convert_len;
    char *in_src = parser->mail_data + m->body_offset;
    int in_len = zmime_body_size(m);
    char *encoding = (char *)zmime_encoding(m);
    char *charset = (char *)zmime_charset(m);
    zbuf_t *zbuf3 = 0;
    char *buf3;
    int buf3_len;

    if (!*encoding) {
        bq = 0;
    } else if (!strcmp(encoding, "base64")) {
        bq = 'b';
    } else if (!strcmp(encoding, "quoted-printable")) {
        bq = 'q';
    } else {
        bq = 0;
    }
    
    if (bq) {
        zbuf3 = zbuf_create(m->body_len>10240?m->body_len:10240);
        if (bq == 'b') {
            buf3_len = zbase64_decode_zbuf(in_src, in_len, zbuf3);
        } else {
            buf3_len = zqp_decode_2045_zbuf(in_src, in_len, zbuf3);
        }
        buf3 = ZBUF_DATA(zbuf3);
        buf3_len = ZBUF_LEN(zbuf3);
    } else {
        buf3 = in_src;
        buf3_len = in_len;
    }

    convert_len = zmime_iconv((*charset)?charset:parser->src_charset_def, buf3, buf3_len, dest);
    if (zbuf3) {
        zbuf_free(zbuf3);
    }

    return convert_len;
}

/* ################################################################## */
static int ___get_header_value(zmime_t *mime, const char *name, char *buf, char **start)
{
    zsize_data_t *sd;
    int rlen, blen=strlen(name);

    ZVECTOR_WALK_BEGIN(mime->header_lines, sd) {
        if ((sd->size>=blen) && (sd->data[blen-1] == ':') && (___CASEEQ_LEN(sd->data, name, blen))) {
            rlen =zmime_header_line_unescape_advanced(sd->data + blen, sd->size - blen, buf);
            buf[rlen] = 0;
            return zskip(buf, rlen, " \t", 0, start);
        }
    } ZVECTOR_WALK_END;
    return 0;
}

char *zmail_get(zmail_t *parser, int module)
{
    zmime_t *mime = parser->top_mime;
    char buf[ZMAIL_HEADER_LINE_MAX_LENGTH + 10], *val, *start;
    int blen, vlen;
    ZSTACK_BUF_FROM(zb, buf, ZMAIL_HEADER_LINE_MAX_LENGTH);
    ZSTACK_BUF(zb2, 1024 * 100);
    
    if (module == 13) {
#define ___mail_hval_first_token(a, b) \
        if (!parser->a) { \
            parser->a = zblank_buffer; \
            blen = ___get_header_value(mime, b, buf, &start); \
            zmime_header_get_first_token(start, blen, &val, &vlen); \
            if (vlen > 0) { \
                parser->a = zmpool_memdupnull(parser->mpool, val, vlen); \
            } \
        } \
        return parser->a; 
        ___mail_hval_first_token(message_id, "message-id:");
    } else if((module == 1) || (module == 2)) {
        if (!parser->subject) {
            parser->subject = zblank_buffer;
            blen = ___get_header_value(mime, "subject:", buf, &start);
            if (blen > 0) {
                parser->subject = zmpool_memdupnull(parser->mpool, start, blen);
            }
        }
        if ((module == 2) && (!parser->subject_utf8)) {
            parser->subject_utf8 = zblank_buffer;
            zmime_header_line_get_utf8(parser->src_charset_def, parser->subject, strlen(parser->subject), zb2);
            parser->subject_utf8 = zmpool_memdupnull(parser->mpool, ZBUF_DATA(zb2), ZBUF_LEN(zb2));
        }
        if (module == 1) {
            return parser->subject;
        } else {
            return parser->subject_utf8;
        }
    } else if((module == 3) || (module == 4)) {
        if (!parser->date) {
            parser->date = zblank_buffer;
            blen = ___get_header_value(mime, "date:", buf, &start);
            if (blen > 0) {
                parser->date = zmpool_memdupnull(parser->mpool, start, blen);
            }
        }
        if (module == 4) {
            parser->date_unix = zmime_header_date_decode(parser->date);
        }
        return parser->date;
    } else if((module == 5) || (module == 105)) {
        if (!parser->from_flag) {
            parser->from_flag = 1;
            blen = ___get_header_value(mime, "from:", buf, &start);
            if (blen > 0) {
                parser->from = (zmime_address_t *)zmime_address_decode_MPOOL(parser->mpool, start, blen);
            }
        }
        if ((module == 105) && (parser->from_flag !=2 )) {
            parser->from_flag = 2;
            if (parser->from) {
                zmime_header_line_get_utf8(parser->src_charset_def, parser->from->name, strlen(parser->from->name), zb2);
                parser->from->name_utf8 = zmpool_memdupnull(parser->mpool, ZBUF_DATA(zb2), ZBUF_LEN(zb2));
            }
        }
        return zblank_buffer;
    } else if(module == 6) {
#define ___mail_noutf8(a, b, c) \
        if (!parser->a) { \
            parser->a = 1; \
            blen = ___get_header_value(mime, b, buf, &start); \
            if (blen > 0) { \
                parser->c  = (zmime_address_t *)zmime_address_decode_MPOOL(parser->mpool, start, blen); \
            } \
        } \
        return zblank_buffer;
        ___mail_noutf8(sender_flag, "sender:", sender);
    } else if(module == 7) {
        ___mail_noutf8(reply_to_flag, "reply-to:", reply_to);
    } else if(module == 11) {
        ___mail_noutf8(receipt_flag, "disposition-notification-to:", receipt);
    } else if(module == 12) {
        ___mail_hval_first_token(in_reply_to, "in-reply-to:");
    } else if((module == 8) || (module==108)) {
#define ___mail_tcb_(tcb_flag, tcbname, tcb, md)  \
        if (!parser->tcb_flag) {  \
            parser->tcb_flag = 1; \
            blen = ___get_header_value(mime, tcbname, buf, &start); \
            parser->tcb = (zvector_t *)zmime_address_vector_decode_MPOOL(parser->mpool, start, blen); \
        } \
        if ((module==md) && (parser->tcb_flag!=2)) { \
            parser->tcb_flag = 2; \
            zmime_address_t *addr; \
            ZVECTOR_WALK_BEGIN(parser->tcb, addr) { \
                zbuf_reset(zb2); \
                zmime_header_line_get_utf8(parser->src_charset_def, addr->name, strlen(addr->name), zb2); \
                addr->name_utf8 = zmpool_memdupnull(parser->mpool, ZBUF_DATA(zb2), ZBUF_LEN(zb2)); \
            } ZVECTOR_WALK_END; \
        } \
        return zblank_buffer;
        ___mail_tcb_(to_flag, "to:", to, 108);
    } else if((module == 9) || (module==109)) {
        ___mail_tcb_(cc_flag, "cc:", cc, 109);
    } else if((module == 10) || (module==110)) {
        ___mail_tcb_(bcc_flag, "bcc:", bcc, 110);
    } else if(module == 14) {
        if (!parser->references_flag) {
            parser->references_flag = 1;
            blen = ___get_header_value(mime, "references:", buf, &start);
            if (blen < 1) {
                return zblank_buffer;
            }
            start[blen] = 0;
            zvector_t *rr;
            zstrtok_t stok;
            char *mid;
            rr = zvector_create(128);
            zstrtok_init(&stok, buf);
            while (zstrtok(&stok, "<> \t,\r\n")) {
                if (stok.len < 2) {
                    continue;
                }
                mid = zmpool_memdupnull(parser->mpool, stok.str, stok.len);
                zvector_add(rr, mid);
            }
            if (ZVECTOR_LEN(rr)) {
                parser->references = zvector_create_MPOOL(parser->mpool, ZVECTOR_LEN(rr));
                ZVECTOR_WALK_BEGIN(rr, mid) {
                    zvector_add(parser->references, mid);
                } ZVECTOR_WALK_END;
            }
            zvector_free(rr);
        }
        return zblank_buffer;
    } else if(module == 61) {
        zmail_set_imap_section(parser);
    } else if(module == 62) {
        zmail_mime_classify(parser);
    }
    return zblank_buffer;
}
#undef ___CASEEQ_LEN
