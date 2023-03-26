/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-10
 * ================================
 */

#include "zc.h"
#include "mime.h"

static void *trim_zmpool_memdupnull(zmpool_t * mp, const void *ptr, int n, zbool_t token_mode)
{
    if (n == 0) {
        return zmpool_memdupnull(mp, ptr, 0);
    }
    const char *ps = ptr, *pe = ps + n;
    for (;ps < pe; ps++) {
        if (token_mode) {
            if ((*ps == '<')||(*ps == '>')) {
                continue;
            }
        }
        if (zistrim(*ps)) {
            continue;
        }
        break;
    }
    pe--;
    for (;ps <= pe; pe--) {
        if (token_mode) {
            if ((*pe == '<')||(*pe == '>')) {
                continue;
            }
        }
        if (zistrim(*pe)) {
            continue;
        }
        break;
    }
    pe++;
    return zmpool_memdupnull(mp, ps, pe-ps);
}

zmime_t *zmime_create(zmail_t *parser)
{
    zmpool_t *mpool = parser->mpool;
    zmime_t *m = (zmime_t *)zmpool_calloc(mpool, 1, sizeof(zmime_t));

    m->parser = parser;
    m->type = zblank_buffer;
    m->encoding = zblank_buffer;
    m->charset = zblank_buffer;
    m->disposition = zblank_buffer;
    m->show_name = zblank_buffer;
    m->name = zblank_buffer;
    m->name_utf8 = zblank_buffer;
    m->filename = zblank_buffer;
    m->filename2231 = zblank_buffer;
    m->filename_utf8 = zblank_buffer;
    m->content_id = zblank_buffer;
    m->boundary = zblank_buffer;
    m->imap_section = zblank_buffer;

    zvector_init_mpool(&(m->raw_header_lines), 10, mpool);
    return m;
}

void zmime_free(zmime_t *mime)
{
}

/* ################################################################## */
const char *zmime_get_type(zmime_t *mime)
{
    return mime->type;
}

const char *zmime_get_encoding(zmime_t *mime)
{
    if (mime->encoding_flag) {
        return mime->encoding;
    }
    mime->encoding_flag = 1;
    zbuf_t *tmpbf = zmail_zbuf_cache_require(mime->parser, 128);
    zmime_get_header_line_value(mime, "Content-Transfer-Encoding:", tmpbf, 0);
    if (zbuf_len(tmpbf)) {
        zmime_header_line_decode_content_transfer_encoding_inner(mime->parser, zbuf_data(tmpbf), zbuf_len(tmpbf), &(mime->encoding));
        zstr_tolower(mime->encoding);
    }
    zmail_zbuf_cache_release(mime->parser, tmpbf);
    return mime->encoding;
}

const char *zmime_get_charset(zmime_t *mime)
{
    return mime->charset;
}

const char *zmime_get_disposition(zmime_t *mime)
{
    if (mime->disposition_flag) {
        return mime->disposition;
    }
    mime->disposition_flag = 1;
    zbuf_t *tmpbf = zmail_zbuf_cache_require(mime->parser, 256);
    zmime_get_header_line_value(mime, "Content-Disposition:", tmpbf, 0);
    if (zbuf_len(tmpbf)) {
        int flag = 0;
        zmime_header_line_decode_content_disposition_inner(mime->parser, zbuf_data(tmpbf), zbuf_len(tmpbf), &(mime->disposition), &(mime->filename), &(mime->filename2231), &flag);
        mime->filename2231_with_charset = flag;
    }
    zmail_zbuf_cache_release(mime->parser, tmpbf);
    return mime->disposition;
}

const char *zmime_get_show_name(zmime_t *mime)
{
    if (!mime->show_name_flag) {
        mime->show_name_flag = 1;
        const char *n = zmime_get_filename_utf8(mime);
        if (ZEMPTY(n)) {
            n = zmime_get_name_utf8(mime);
        }
        if (ZEMPTY(n)) {
            n = zmime_get_filename(mime);
        }
        if (ZEMPTY(n)) {
            n = zmime_get_name(mime);
        }
        mime->show_name = (char *)(void *)n;
    }
    return mime->show_name;
}

const char *zmime_get_name(zmime_t *mime)
{
    return mime->name;
}

const char *zmime_get_name_utf8(zmime_t *mime)
{
    if (mime->name_utf8_flag) {
        return mime->name_utf8;
    }
    mime->name_utf8_flag = 1;
    int len = strlen(mime->name);
    if (len > 0) {
        zbuf_t *tmpbf = zmail_zbuf_cache_require(mime->parser, 256);
        zmime_header_line_get_utf8_inner(mime->parser, mime->name, len, tmpbf);
        mime->name_utf8 = trim_zmpool_memdupnull(mime->parser->mpool, zbuf_data(tmpbf), zbuf_len(tmpbf), 0);
        zmail_zbuf_cache_release(mime->parser, tmpbf);
    }
    return mime->name_utf8;
}

const char *zmime_get_filename(zmime_t *mime)
{
    if (!mime->disposition_flag) {
        zmime_get_disposition(mime);
    }
    return mime->filename;
}

const char *zmime_get_filename_utf8(zmime_t *mime)
{
    if (mime->filename_utf8_flag) {
        return mime->filename_utf8;
    }
    mime->filename_utf8_flag = 1;
    if (!mime->disposition_flag) {
        zmime_get_disposition(mime);
    }
    zbuf_t *uname = zmail_zbuf_cache_require(mime->parser, 4096);
    if (!ZEMPTY(mime->filename2231)) {
        zmime_header_line_get_utf8_2231(mime->parser->src_charset_def, mime->filename2231, -1, uname, mime->filename2231_with_charset);
    } else  if (!ZEMPTY(mime->filename)) {
        zmime_header_line_get_utf8_inner(mime->parser, mime->filename, -1, uname);
    }
    mime->filename_utf8 = trim_zmpool_memdupnull(mime->parser->mpool, zbuf_data(uname), zbuf_len(uname), 0);
    zmail_zbuf_cache_release(mime->parser, uname);
    return mime->filename_utf8;
}

const char *zmime_get_filename2231(zmime_t *mime, zbool_t *with_charset_flag)
{
    if (mime->filename2231_flag) {
        if (with_charset_flag) {
            *with_charset_flag = mime->filename2231_with_charset;
        }
        return mime->filename2231;
    }
    mime->filename2231_flag = 1;
    if (!mime->disposition_flag) {
        zmime_get_disposition(mime);
    }
    if (with_charset_flag) {
        *with_charset_flag = mime->filename2231_with_charset;
    }
    return mime->filename2231;
}

const char *zmime_get_content_id(zmime_t *mime)
{
    if (mime->content_id_flag == 0) {
        mime->content_id_flag = 1;
        zbuf_t *tmpbf = zmail_zbuf_cache_require(mime->parser, 128);
        zmime_get_header_line_value(mime, "Content-ID:", tmpbf, 0);
        int zmime_header_line_get_first_token_inner(const char *line_, int in_len, char **val);
        char *v;
        int len = zmime_header_line_get_first_token_inner(zbuf_data(tmpbf), zbuf_len(tmpbf), &v);
        if (len > 0) {
            mime->content_id = trim_zmpool_memdupnull(mime->parser->mpool, v, len, 1);
        }
        zmail_zbuf_cache_release(mime->parser, tmpbf);
    }
    return mime->content_id;
}

const char *zmime_get_boundary(zmime_t *mime)
{
    return mime->boundary;
}

const char *zmime_get_imap_section(zmime_t *mime)
{
    if (!mime->parser->section_flag) {
        void zmail_set_imap_section(zmail_t *);
        zmail_set_imap_section(mime->parser);
    }
    mime->parser->section_flag = 1;
    return mime->imap_section;
}

const char *zmime_get_header_data(zmime_t *mime)
{
    return mime->parser->mail_data + mime->header_offset;
}

int zmime_get_header_offset(zmime_t *mime)
{
    return mime->header_offset;
}

int zmime_get_header_len(zmime_t *mime)
{
    return mime->header_len;
}

const char *zmime_get_body_data(zmime_t *mime)
{
    return mime->parser->mail_data + mime->body_offset;
}

int zmime_get_body_offset(zmime_t *mime)
{
    return mime->body_offset;
}

int zmime_get_body_len(zmime_t *mime)
{
    return mime->body_len;
}

zbool_t zmime_is_tnef(zmime_t *mime)
{
    return mime->is_tnef;
}

zmime_t *zmime_next(zmime_t *mime)
{
    return mime->next;
}

zmime_t *zmime_child(zmime_t *mime)
{
    return mime->child_head;
}

zmime_t *zmime_parent(zmime_t *mime)
{
    return mime->parent;
}

const zvector_t *zmime_get_raw_header_line_vector(zmime_t *mime)
{
    return &(mime->raw_header_lines);
}

int zmime_get_raw_header_line_ptr(zmime_t *mime, const char *header_name, char **result, int sn)
{
    int firstch, nn = 0;
    int name_len;
    zsize_data_t *sdn = 0;

    name_len = strlen(header_name);
    if (name_len == 0) {
        return -1;
    }
    if (header_name[name_len-1] == ':') {
        name_len --;
    }
    if (name_len == 0) {
        return -1;
    }

    firstch = ztoupper(header_name[0]);
    ZVECTOR_WALK_BEGIN(&(mime->raw_header_lines), zsize_data_t *, sd) {
        if (sd->size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd->data);
        if (data[name_len] != ':') {
            continue;
        }
        if (firstch != ztoupper(data[0])) {
            continue;
        }
        if (strncasecmp(data, header_name, name_len)) {
            continue;
        }
        if (nn == sn) {
            *result = sd->data;
            return sd->size;
        }
        nn ++;
        sdn = sd;
    } ZVECTOR_WALK_END;
    if (!sdn) {
        return -1;
    }
    if (sn != -1) {
        return -1;
    }

    *result = sdn->data;
    return sdn->size;
}

int zmime_get_raw_header_line(zmime_t *mime, const char *header_name, zbuf_t *result, int sn)
{
    zbuf_reset(result);
    char *vp;
    int vlen = zmime_get_raw_header_line_ptr(mime, header_name, &vp, sn);
    if (vlen > 0) {
        zbuf_memcpy(result, vp, vlen);
    }
    return vlen;
}

int zmime_get_header_line_value(zmime_t *mime, const char *header_name, zbuf_t *result, int n)
{
    zbuf_reset(result);
    int len = strlen(header_name);
    if (len < 1) {
        return -1;
    }
    if (header_name[len-1] == ':') {
        len --;
    }
    if (len < 1) {
        return -1;
    }
    char *vp;
    int vlen = zmime_get_raw_header_line_ptr(mime, header_name, &vp, n);
    if (vlen < 0) {
        return vlen;
    }
    if (vlen > 0) {
        zbuf_t *tmpbf = zmail_zbuf_cache_require(mime->parser, 4096);
        zmime_raw_header_line_unescape(vp + (len + 1), vlen - (len + 1), tmpbf);
        char *start;
        int rlen = zskip(zbuf_data(tmpbf), zbuf_len(tmpbf), " \t\r\n", 0, &start);
        if (rlen > 0) {
            zbuf_memcpy(result, start, rlen);
        }
        zmail_zbuf_cache_release(mime->parser, tmpbf);
    }
    return zbuf_len(result);
}

void zmime_get_decoded_content(zmime_t *mime, zbuf_t *result)
{
    char *in_src = mime->parser->mail_data + mime->body_offset;
    int in_len = mime->body_len;
    const char *enc = zmime_get_encoding(mime);

    if (!ZEMPTY(enc)) {
        if (!strcmp(enc, "base64")) {
            zbase64_decode(in_src, in_len, result);
            return;
        } else if (!strcmp(enc, "quoted-printable")) {
            zqp_decode_2045(in_src, in_len, result);
            return;
        }
    }
    zbuf_memcpy(result, in_src, in_len);
}

void zmime_get_decoded_content_utf8(zmime_t *mime, zbuf_t *result)
{
    char *in_src = mime->parser->mail_data + mime->body_offset;
    int in_len = mime->body_len;
    const char *enc = zmime_get_encoding(mime);
    const char *cha = zmime_get_charset(mime);
    const char *str;
    int str_len;
    zbuf_t *tmp_cache_buf = 0;
    zbuf_reset(result);

    if ((!strcmp(enc, "base64")) || (!strcmp(enc, "quoted-printable"))) {
        tmp_cache_buf = zmail_zbuf_cache_require(mime->parser, in_len < 128?128:in_len);
        if (*enc == 'b') {
            zbase64_decode(in_src, in_len, tmp_cache_buf);
        } else {
            zqp_decode_2045(in_src, in_len, tmp_cache_buf);
        }
        str = zbuf_data(tmp_cache_buf);
        str_len = zbuf_len(tmp_cache_buf);
    } else {
        str = in_src;
        str_len = in_len;
    }

    zmime_iconv((ZEMPTY(cha))?mime->parser->src_charset_def:cha, str, str_len, result);
    if (tmp_cache_buf) {
        zmail_zbuf_cache_release(mime->parser, tmp_cache_buf);
    }
}


/* ################################################################## */
static zmail_t *zmail_create_parser_from_data_prepare(const char *mail_data, int mail_data_len, const char *default_charset)
{
    zmpool_t *mpool = zmpool_create_greedy_pool(102400, 10240);
    zmail_t *parser = zmpool_calloc(mpool, 1, sizeof(zmail_t));
    parser->mpool = mpool;

    parser->mail_data = (char *)mail_data;
    parser->mail_size = mail_data_len;
    parser->src_charset_def = zmpool_strdup(parser->mpool, default_charset);

    parser->date_unix = -2;
    parser->all_mimes = zvector_create(128);

    parser->subject = zblank_buffer;
    parser->subject_utf8 = zblank_buffer;
    parser->date = zblank_buffer;
    parser->in_reply_to = zblank_buffer;
    parser->message_id = zblank_buffer;

    return parser;
}

zmail_t *zmail_create_parser_from_data(const char *mail_data, int mail_data_len, const char *default_charset)
{
    zmail_t *parser = zmail_create_parser_from_data_prepare(mail_data, mail_data_len, default_charset);
    zmail_decode_mime_inner(parser);
    return parser;
}

zmail_t *zmail_create_parser_from_pathname(const char *pathname, const char *default_charset)
{
    zmmap_reader_t *fmmap = (zmmap_reader_t *)zmalloc(sizeof(zmmap_reader_t));
    if (zmmap_reader_init(fmmap, pathname) < 0) {
        zfree(fmmap);
        return 0;
    }
    zmail_t *parser = zmail_create_parser_from_data(fmmap->data, fmmap->len, default_charset);
    parser->fmmap = fmmap;
    return parser;
}

void zmail_free(zmail_t *parser)
{
    if (parser->fmmap) {
        zmmap_reader_fini(parser->fmmap);
        zfree(parser->fmmap);
    }
    zvector_free(parser->all_mimes);
    zmail_zbuf_cache_release_all(parser);
    zmpool_free_pool(parser->mpool);
}

const char *zmail_get_data(zmail_t *parser)
{
    return parser->mail_data;
}

int zmail_get_len(zmail_t *parser)
{
    return parser->mail_size;
}

const char *zmail_get_header_data(zmail_t *parser)
{
    return parser->mail_data + parser->top_mime->header_offset;
}

int zmail_get_header_offset(zmail_t *parser)
{
    return parser->top_mime->header_offset;
}

int zmail_get_header_len(zmail_t *parser)
{
    return parser->top_mime->header_len;
}

const char *zmail_get_body_data(zmail_t *parser)
{
    return parser->mail_data + parser->top_mime->body_offset;
}


int zmail_get_body_offset(zmail_t *parser)
{
    return parser->top_mime->body_offset;
}

int zmail_get_body_len(zmail_t *parser)
{
    return parser->top_mime->body_len;
}

const char *zmail_get_message_id(zmail_t *parser)
{
    if (parser->message_id_flag == 0) {
        parser->message_id_flag = 1;
        zbuf_t *tmpbf = zmail_zbuf_cache_require(parser, 128);
        zmime_get_header_line_value(parser->top_mime, "Message-ID:", tmpbf, 0);
        int zmime_header_line_get_first_token_inner(const char *line, int in_len, char **val);
        char *v;
        int len = zmime_header_line_get_first_token_inner(zbuf_data(tmpbf), zbuf_len(tmpbf), &v);
        if (len > 0) {
            parser->message_id = trim_zmpool_memdupnull(parser->mpool, v, len, 1);
        }
        zmail_zbuf_cache_release(parser, tmpbf);
    }
    return parser->message_id;
}

const char *zmail_get_subject(zmail_t *parser)
{
    if (!parser->subject_flag) {
        parser->subject_flag = 1;
        zbuf_t *tmpbf = zmail_zbuf_cache_require(parser, 256);
        if (zmime_get_header_line_value(parser->top_mime, "Subject:", tmpbf, 0) > -1) {
            if (zbuf_len(tmpbf)) {
                parser->subject = trim_zmpool_memdupnull(parser->mpool, zbuf_data(tmpbf), zbuf_len(tmpbf), 0);
            }
        }
        zmail_zbuf_cache_release(parser, tmpbf);
    }
    return parser->subject;
}

const char *zmail_get_subject_utf8(zmail_t *parser)
{
    if (!parser->subject_flag) {
        zmail_get_subject(parser);
    }
    if (!parser->subject_utf8_flag) {
        parser->subject_utf8_flag = 1;
        int len = strlen(parser->subject);
        if (len > 0) {
            zbuf_t *tmpbf = zmail_zbuf_cache_require(parser, 256);
            zmime_header_line_get_utf8_inner(parser, parser->subject, len, tmpbf);
            parser->subject_utf8 = trim_zmpool_memdupnull(parser->mpool, zbuf_data(tmpbf), zbuf_len(tmpbf), 0);
            zmail_zbuf_cache_release(parser, tmpbf);
        }
    }
    return parser->subject_utf8;
}

const char *zmail_get_date(zmail_t *parser)
{
    if (!parser->date_flag) {
        parser->date_flag = 1;
        zbuf_t *tmpbf = zmail_zbuf_cache_require(parser, 256);
        if (zmime_get_header_line_value(parser->top_mime, "Date:", tmpbf, 0) > -1) {
            if (zbuf_len(tmpbf)) {
                parser->date = trim_zmpool_memdupnull(parser->mpool, zbuf_data(tmpbf), zbuf_len(tmpbf), 1);
            }
        }
        zmail_zbuf_cache_release(parser, tmpbf);
    }
    return parser->date;
}

long zmail_get_date_unix(zmail_t *parser)
{
    if (!parser->date_flag) {
        zmail_get_date(parser);
    }
    if(parser->date_unix == -2) {
        parser->date_unix = -1;
        if (parser->date) {
            parser->date_unix = zmime_header_line_decode_date(parser->date);
        }
    }
    return parser->date_unix;
}

const zmime_address_t *zmail_get_from(zmail_t *parser)
{
#define ___mail_noutf8(a, b, c) \
    if (!parser->a) { \
        parser->a = 1; \
        parser->c = (zmime_address_t *)zmpool_malloc(parser->mpool, sizeof(zmime_address_t)); \
        parser->c->name = zblank_buffer; \
        parser->c->address = zblank_buffer; \
        parser->c->name_utf8 = zblank_buffer; \
        zbuf_t *tmpbf = zmail_zbuf_cache_require(parser, 256); \
        zmime_get_header_line_value(parser->top_mime, b, tmpbf, 0); \
        if (zbuf_len(tmpbf)) { \
            zvector_t *vec = zmime_header_line_get_address_vector_inner(parser, zbuf_data(tmpbf), zbuf_len(tmpbf)); \
            if (vec && zvector_len(vec)) { \
                parser->c->name =((zmime_address_t *)(vec->data[0]))->name; \
                parser->c->address = ((zmime_address_t *)(vec->data[0]))->address; \
                parser->c->name_utf8 = ((zmime_address_t *)(vec->data[0]))->name_utf8; \
            } \
        } \
        zmail_zbuf_cache_release(parser, tmpbf); \
    } \
    return parser->c;
    ___mail_noutf8(from_flag, "From:", from);
}

const zmime_address_t *zmail_get_from_utf8(zmail_t *parser)
{
    if (!parser->from_flag) {
        zmail_get_from(parser);
    }
    if (parser->from_flag != 2){
        parser->from_flag = 2;
        int len = 0;
        if (parser->from)
        {
            if (zempty(parser->from->name_utf8))
            {
                len = strlen(parser->from->name);
            }
            if (len > 0)
            {
                zbuf_t *tmpbf = zmail_zbuf_cache_require(parser, 256);
                zmime_header_line_get_utf8_inner(parser, parser->from->name, len, tmpbf);
                parser->from->name_utf8 = trim_zmpool_memdupnull(parser->mpool, zbuf_data(tmpbf), zbuf_len(tmpbf), 0);
                zmail_zbuf_cache_release(parser, tmpbf);
            }
        }
    }
    return parser->from;
}

const zmime_address_t *zmail_get_sender(zmail_t *parser)
{
    ___mail_noutf8(sender_flag, "Sender:", sender);
}

const zmime_address_t *zmail_get_reply_to(zmail_t *parser)
{
    ___mail_noutf8(reply_to_flag, "Reply-To:", reply_to);
}

const zmime_address_t *zmail_get_receipt(zmail_t *parser)
{
    ___mail_noutf8(receipt_flag, "Disposition-Notification-To:", receipt);
}

const char *zmail_get_in_reply_to(zmail_t *parser)
{
    if (parser->in_reply_to_flag == 0) {
        parser->in_reply_to_flag = 1;
        ZSTACK_BUF(tmpbf, 1024);
        zmime_get_header_line_value(parser->top_mime, "In-Reply-To:", tmpbf, 0);
        int zmime_header_line_get_first_token_inner(const char *line, int in_len, char **val);
        char *v;
        int len = zmime_header_line_get_first_token_inner(zbuf_data(tmpbf), zbuf_len(tmpbf), &v);
        if (len > 0) {
            parser->in_reply_to = trim_zmpool_memdupnull(parser->mpool, v, len, 1);
        }
    }
    return parser->in_reply_to;
}

const zvector_t *zmail_get_to(zmail_t *parser) /* zmime_address_t* */
{
#define ___mail_parser_engine_tcb(tcb_flag, tcbname, tcbnamelen, tcb, utf8_tf)  \
    zbuf_t *tmpbf = 0, *line = 0; \
    if (!parser->tcb_flag) {  \
        parser->tcb_flag = 1; \
        zvector_t *raw_header_lines = &(parser->top_mime->raw_header_lines); \
        ZVECTOR_WALK_BEGIN(raw_header_lines, zsize_data_t *, sdp) { \
            if (sdp->size <= tcbnamelen)  { continue; } \
            if (!ZSTR_N_CASE_EQ(sdp->data, tcbname, tcbnamelen)) { continue; } \
            if (tmpbf == 0) { tmpbf = zmail_zbuf_cache_require(parser, 256); } \
            if (line == 0) { line = zmail_zbuf_cache_require(parser, 256); } \
            zbuf_reset(tmpbf); \
            zmime_raw_header_line_unescape(sdp->data + tcbnamelen, sdp->size - tcbnamelen, tmpbf); \
            zbuf_puts(line, ", "); \
            zbuf_append(line, tmpbf); \
        } ZVECTOR_WALK_END; \
        if (line) { \
            parser->tcb=zmime_header_line_get_address_vector_inner(parser, zbuf_data(line),zbuf_len(line)); \
        } else { \
            parser->tcb=zmime_header_line_get_address_vector_inner(parser, "", 0); \
        }\
    } \
    if (utf8_tf && (parser->tcb_flag!=2)) { \
        parser->tcb_flag = 2; \
        ZVECTOR_WALK_BEGIN(parser->tcb, zmime_address_t *, addr) { \
            int len = strlen(addr->name); \
            if (len>0) { \
                if (tmpbf == 0) { tmpbf = zmail_zbuf_cache_require(parser, 256); } \
                zbuf_reset(tmpbf); \
                zmime_header_line_get_utf8_inner(parser, addr->name, len, tmpbf); \
                addr->name_utf8 = trim_zmpool_memdupnull(parser->mpool, zbuf_data(tmpbf), zbuf_len(tmpbf), 0); \
            } \
        } ZVECTOR_WALK_END; \
    } \
    if (tmpbf) { zmail_zbuf_cache_release(parser, tmpbf); } \
    if (line) { zmail_zbuf_cache_release(parser, line); } \
    return parser->tcb;
    ___mail_parser_engine_tcb(to_flag, "To:", 3, to, 0);
}

const zvector_t *zmail_get_to_utf8(zmail_t *parser)
{
    ___mail_parser_engine_tcb(to_flag, "To:", 3, to, 1);
}

const zvector_t *zmail_get_cc(zmail_t *parser)
{
    ___mail_parser_engine_tcb(cc_flag, "Cc:", 3, cc, 0);
}

const zvector_t *zmail_get_cc_utf8(zmail_t *parser)
{
    ___mail_parser_engine_tcb(cc_flag, "Cc:", 3, cc, 1);
}

const zvector_t *zmail_get_bcc(zmail_t *parser)
{
    ___mail_parser_engine_tcb(bcc_flag, "Bcc:", 4, bcc, 0);
}

const zvector_t *zmail_get_bcc_utf8(zmail_t *parser)
{
    ___mail_parser_engine_tcb(bcc_flag, "Bcc:", 4, bcc, 1);
}

const zargv_t *zmail_get_references(zmail_t *parser)
{
    if (parser->references_flag) {
        return parser->references;
    }
    parser->references_flag = 1;
    zbuf_t *tmpbf = zmail_zbuf_cache_require(parser, 1024);
    zmime_get_header_line_value(parser->top_mime, "References:", tmpbf, 0);
    parser->references = (zargv_t *)zmpool_malloc(parser->mpool, sizeof(zargv_t) + sizeof(zmpool_t *));
    if (zbuf_len(tmpbf)) {
        int count = 10;
        char *p = zbuf_data(tmpbf);
        for (; *p; p++) {
            if (*p == '<') {
                count++;
            }
        }
        zargv_init_mpool(parser->references, count, parser->mpool);
        zargv_split_append(parser->references, zbuf_data(tmpbf), "<> \t,\r\n");
    } else {
        zargv_init_mpool(parser->references, 0, parser->mpool);
    }
    zmail_zbuf_cache_release(parser, tmpbf);
    return parser->references;
}

const zmime_t *zmail_get_top_mime(zmail_t *parser)
{
    return parser->top_mime;
}

const zvector_t *zmail_get_all_mimes(zmail_t *parser)
{
    return parser->all_mimes;
}

const zvector_t *zmail_get_text_mimes(zmail_t *parser)
{
    if(!parser->classify_flag) {
        void zmime_classify(zmail_t * parser);
        zmime_classify(parser);
    }
    return parser->text_mimes;
}

const zvector_t *zmail_get_show_mimes(zmail_t *parser)
{
    if(!parser->classify_flag) {
        void zmime_classify(zmail_t * parser);
        zmime_classify(parser);
    }
    return parser->show_mimes;
}
const zvector_t *zmail_get_attachment_mimes(zmail_t *parser)
{
    if(!parser->classify_flag) {
        void zmime_classify(zmail_t * parser);
        zmime_classify(parser);
    }
    return parser->attachment_mimes;
}

/* n == 0: first, n == -1: last */
const zvector_t *zmail_get_raw_header_line_vector(zmail_t *parser)
{
    return zmime_get_raw_header_line_vector(parser->top_mime);
}

int zmail_get_raw_header_line(zmail_t *parser, const char *header_name, zbuf_t *result, int sn)
{
    return zmime_get_raw_header_line(parser->top_mime, header_name, result, sn);
}

int zmail_get_header_line_value(zmail_t *parser, const char *header_name, zbuf_t *result, int sn)
{
    return zmime_get_header_line_value(parser->top_mime, header_name, result, sn);
}
