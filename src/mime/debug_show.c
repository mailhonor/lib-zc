
/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-01-13
 * ================================
 */

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif // __GNUC__

#include "./mime.h"

static void _debug_show_addr(zmail_t *parser, const char *h, const zmime_address_t *addr)
{
    const char *fmt = "%15s: %s";
    zbuf_t *tmpstr = zmail_zbuf_cache_require(parser, 128); 
    zmail_get_raw_header_line(parser, h, tmpstr, 0);
    zdebug_show(fmt, h, zbuf_data(tmpstr));
    zdebug_show(fmt, "", addr->address);
    zdebug_show(fmt, "", addr->name);
    zdebug_show(fmt, "", addr->name_utf8);
    zmail_zbuf_cache_release(parser, tmpstr);
}

static void _debug_show_addr_vector(zmail_t *parser, const char *h, const zvector_t *address_vec)
{
    const char *fmt = "%15s: %s";
    char nh[256];
    int i = 0;

    zbuf_t *tmpstr = zmail_zbuf_cache_require(parser, 128); 
    zmail_get_raw_header_line(parser, h, tmpstr, 0);
    zdebug_show(fmt, h, zbuf_data(tmpstr));
    
    ZVECTOR_WALK_BEGIN(address_vec, zmime_address_t *, addr) {
        if (i == 0) {
            zsprintf(nh, "%s (1)", h);
        } else {
            zsprintf(nh, "(%d)", i+1);
        }
        i ++;
        zdebug_show(fmt, nh, addr->address);
        zdebug_show(fmt, "", addr->name);
        zdebug_show(fmt, "", addr->name_utf8);
    } ZVECTOR_WALK_END;
    zmail_zbuf_cache_release(parser, tmpstr);
}

void zmail_debug_show(zmail_t *parser)
{
    const char *fmt = "%15s: %s\n";
    int i = 0;
    zbuf_t *tmpstr = zmail_zbuf_cache_require(parser, 128); 

    zdebug_show(fmt, "Date", zmail_get_date(parser));
    zdebug_show("%15s: %zd","", zmail_get_date_unix(parser));
    if (1) {
        ssize_t t = zmail_get_date_unix_by_received(parser);
        zdebug_show("%15s: %zd", "Received Date", t);
        char buf[zvar_rfc1123_date_string_size + 1];
        zbuild_rfc822_date_string(t, buf);
        zdebug_show("%15s: %s", "Received Date", buf);
    }
    zdebug_show("");
    zdebug_show(fmt, "Subject", zmail_get_subject(parser));
    zdebug_show(fmt, "Subject_utf8", zmail_get_subject_utf8(parser));
    zdebug_show("");
    _debug_show_addr(parser, "From", zmail_get_from_utf8(parser));
    zdebug_show("");
    _debug_show_addr(parser, "Sender", zmail_get_sender(parser));
    zdebug_show("");
    _debug_show_addr(parser, "Reply-To", zmail_get_reply_to(parser));
    zdebug_show("");
    _debug_show_addr(parser, "Disposition-Notification-To", zmail_get_receipt(parser));
    zdebug_show("");
    zdebug_show(fmt, "In-Reply-To", zmail_get_in_reply_to(parser));
    zdebug_show("");
    _debug_show_addr_vector(parser, "To", zmail_get_to_utf8(parser));
    zdebug_show("");
    _debug_show_addr_vector(parser, "Cc", zmail_get_cc_utf8(parser));
    zdebug_show("");
    _debug_show_addr_vector(parser, "Bcc", zmail_get_bcc_utf8(parser));
    zdebug_show("");
    zdebug_show(fmt, "Message-Id", zmail_get_message_id(parser));

    {
        i = 0;
        zdebug_show("");
        zmail_get_raw_header_line(parser, "References", tmpstr, 0);
        zdebug_show(fmt, "References", zbuf_data(tmpstr));
        const zargv_t *references = zmail_get_references(parser);
        ZARGV_WALK_BEGIN(references, r) {
            if (i==0) {
                zdebug_show(fmt, "References", r);
            } else {
                zdebug_show(fmt, "", r);
            }
            i++;
        } ZARGV_WALK_END;
    }

    const zvector_t *allm = zmail_get_all_mimes(parser);
    i = 0;
    ZVECTOR_WALK_BEGIN(allm, zmime_t *, m) {
        zdebug_show("");
        char buf[128];
        zsprintf(buf, "Mime (%d)", i+1);
        zdebug_show(fmt, buf, zmime_get_type(m));
        zmime_get_raw_header_line(m, "Content-Type", tmpstr, 0);
        zdebug_show(fmt, "Content-Type", zbuf_data(tmpstr));
        zmime_get_raw_header_line(m, "Content-Transfer-Encoding", tmpstr, 0);
        zdebug_show(fmt, "Content-Transfer-Encoding", zbuf_data(tmpstr));
        zmime_get_raw_header_line(m, "Content-Disposition", tmpstr, 0);
        zdebug_show(fmt, "Content-Disposition", zbuf_data(tmpstr));
        zdebug_show(fmt, "disposition", zmime_get_disposition(m));
        zdebug_show(fmt, "name", zmime_get_name(m));
        zdebug_show(fmt, "name_utf8", zmime_get_name_utf8(m));
        zdebug_show(fmt, "filename", zmime_get_filename(m));
        zdebug_show(fmt, "filename2231", zmime_get_filename2231(m, 0));
        zdebug_show(fmt, "filename_utf8", zmime_get_filename_utf8(m));
        zdebug_show(fmt, "content_id", zmime_get_content_id(m));
        zdebug_show(fmt, "encoding", zmime_get_encoding(m));
        zdebug_show(fmt, "boundary", zmime_get_boundary(m));
        zdebug_show(fmt, "section", zmime_get_imap_section(m));
        zsprintf(buf, "%d,%d,%d,%d", zmime_get_header_offset(m), zmime_get_header_len(m), zmime_get_body_offset(m), zmime_get_body_len(m));
        zdebug_show(fmt, "", buf);
        i++;
    } ZVECTOR_WALK_END;

    i = 0;
    const zvector_t *textm = zmail_get_text_mimes(parser);
    ZVECTOR_WALK_BEGIN(textm, zmime_t *, m) {
        zdebug_show("");
        zmime_get_decoded_content_utf8(m, tmpstr);
        zdebug_show(fmt, zmime_get_type(m), zbuf_data(tmpstr));
    } ZVECTOR_WALK_END;

    zmail_zbuf_cache_release(parser, tmpstr);
}
